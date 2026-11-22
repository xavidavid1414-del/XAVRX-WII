#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace xavrx::system {

// Deterministic SD memory-card command model. This is a protocol-level
// foundation, not a cycle-accurate SDHCI/SDIO implementation.
class SdCardProtocol final {
public:
    static constexpr std::size_t block_size = 512U;
    enum class State : std::uint8_t { idle, ready, standby, transfer };
    struct Response {
        bool accepted{false};
        std::uint8_t r1{0x04U}; // idle-state bit is set after power-up
        std::uint32_t value{0U};
        std::array<std::uint8_t, 16> long_response{};
    };

    [[nodiscard]] bool attach(std::vector<std::uint8_t> image) {
        if (image.empty() || image.size() % block_size != 0U ||
            image.size() / block_size > 0xffffffffULL) return false;
        image_ = std::move(image);
        reset();
        return true;
    }
    void detach() noexcept { image_.clear(); reset(); }
    void reset() noexcept {
        state_ = State::idle; app_command_ = false; selected_ = false;
        transfer_active_ = false; transfer_write_ = false; multi_block_ = false;
        transfer_block_ = 0U;
        block_length_ = static_cast<std::uint32_t>(block_size);
        last_command_ = 0U;
    }
    [[nodiscard]] State state() const noexcept { return state_; }
    [[nodiscard]] bool present() const noexcept { return !image_.empty(); }
    [[nodiscard]] std::uint32_t blocks() const noexcept {
        return static_cast<std::uint32_t>(image_.size() / block_size);
    }

    [[nodiscard]] Response command(std::uint8_t index, std::uint32_t argument) noexcept {
        Response out{};
        out.r1 = state_ == State::idle ? 0x01U : 0x00U;
        if (image_.empty()) { out.r1 |= 0x04U; return out; }
        const bool was_app = app_command_;
        app_command_ = false;
        last_command_ = index;

        if (index == 0U) { reset(); out.accepted = true; out.r1 = 0x01U; return out; }
        if (index == 8U) {
            // This model represents an SDHC card using the standard 2.7-3.6 V range.
            // CMD8 reserves bits 31:12 and carries VHS in 11:8 plus the host check pattern.
            if (state_ != State::idle || (argument & 0xfffff000U) != 0U ||
                ((argument >> 8U) & 0x0fU) != 0x01U) return illegal(out);
            out.accepted = true; out.r1 = 0x01U; out.value = argument & 0xfffU; return out;
        }
        if (index == 55U) {
            if (state_ == State::idle || state_ == State::ready || state_ == State::standby || state_ == State::transfer) {
                app_command_ = true; out.accepted = true; out.r1 = state_ == State::idle ? 0x01U : 0x00U;
                return out;
            }
            return illegal(out);
        }
        if (was_app && index == 41U) {
            constexpr std::uint32_t supported_voltage_window = 0x00ff8000U;
            constexpr std::uint32_t host_capacity_support = 1U << 30U;
            if ((state_ != State::idle && state_ != State::ready) ||
                (argument & supported_voltage_window) == 0U ||
                (argument & host_capacity_support) == 0U) return illegal(out);
            state_ = State::ready; out.accepted = true; out.r1 = 0x00U; out.value = 0xC0FF8000U; return out;
        }
        switch (index) {
        case 2U: // CID
            if (state_ != State::ready) return illegal(out);
            state_ = State::standby; out.accepted = true; out.r1 = 0U;
            out.long_response = {0x03U,0x58U,0x41U,0x56U,0x52U,0x58U,0x57U,0x49U,0x49U,0x01U,0x26U,0x11U,0x00U,0x00U,0x01U,0x00U};
            return out;
        case 3U: // RCA
            if (state_ != State::standby) return illegal(out);
            out.accepted = true; out.r1 = 0U; out.value = 0x12340000U; return out;
        case 7U:
            if ((argument >> 16U) == 0U) {
                if (state_ != State::transfer && state_ != State::standby) return illegal(out);
                selected_ = false; state_ = State::standby;
                transfer_active_ = false; transfer_write_ = false; multi_block_ = false;
                transfer_block_ = 0U;
            }
            else if ((argument >> 16U) == 0x1234U && state_ == State::standby) { selected_ = true; state_ = State::transfer; }
            else return illegal(out);
            out.accepted = true; out.r1 = 0U; return out;
        case 9U: // CSD
            if (!selected_) return illegal(out);
            out.accepted = true; out.r1 = 0U; out.long_response[0] = 0x40U;
            out.long_response[1] = static_cast<std::uint8_t>((blocks() >> 24U) & 0xffU);
            out.long_response[2] = static_cast<std::uint8_t>((blocks() >> 16U) & 0xffU);
            out.long_response[3] = static_cast<std::uint8_t>((blocks() >> 8U) & 0xffU);
            out.long_response[4] = static_cast<std::uint8_t>(blocks() & 0xffU); return out;
        case 13U: // SEND_STATUS
            if (!selected_) return illegal(out);
            out.accepted = true; out.r1 = 0U; out.value = 0U; return out;
        case 16U:
            if (!selected_ || argument != block_size) return illegal(out);
            block_length_ = argument; out.accepted = true; out.r1 = 0U; return out;
        case 17U: case 18U: case 24U: case 25U:
            if (!selected_ || state_ != State::transfer || block_length_ != block_size ||
                argument >= blocks()) return illegal(out);
            transfer_block_ = argument; transfer_active_ = true;
            transfer_write_ = index == 24U || index == 25U;
            multi_block_ = index == 18U || index == 25U;
            out.accepted = true; out.r1 = 0U; return out;
        case 12U:
            transfer_active_ = false; multi_block_ = false;
            out.accepted = selected_; out.r1 = selected_ ? 0U : 0x04U; return out;
        default: return illegal(out);
        }
    }

    [[nodiscard]] bool read_data_block(std::span<std::uint8_t> output) noexcept {
        if (!transfer_active_ || transfer_write_ || output.size() != block_size ||
            transfer_block_ >= blocks()) return false;
        const auto offset = static_cast<std::size_t>(transfer_block_) * block_size;
        for (std::size_t i = 0; i < block_size; ++i) output[i] = image_[offset + i];
        advance_transfer(); return true;
    }
    [[nodiscard]] bool write_data_block(std::span<const std::uint8_t> input) noexcept {
        if (!transfer_active_ || !transfer_write_ || input.size() != block_size ||
            transfer_block_ >= blocks()) return false;
        const auto offset = static_cast<std::size_t>(transfer_block_) * block_size;
        for (std::size_t i = 0; i < block_size; ++i) image_[offset + i] = input[i];
        advance_transfer(); return true;
    }
private:
    [[nodiscard]] static Response illegal(Response out) noexcept {
        out.accepted = false; out.r1 |= 0x04U; return out;
    }
    void advance_transfer() noexcept {
        if (!multi_block_) { transfer_active_ = false; return; }
        if (transfer_block_ + 1U >= blocks()) { transfer_active_ = false; return; }
        ++transfer_block_;
    }
    std::vector<std::uint8_t> image_;
    State state_{State::idle};
    bool app_command_{false}, selected_{false}, transfer_active_{false};
    bool transfer_write_{false}, multi_block_{false};
    std::uint32_t block_length_{static_cast<std::uint32_t>(block_size)};
    std::uint32_t transfer_block_{0U}, last_command_{0U};
};
} // namespace xavrx::system

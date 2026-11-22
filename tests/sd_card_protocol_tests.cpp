#include "xavrx/system/sd_card_protocol.hpp"
#include <array>
#include <cstdint>
#include <iostream>
#include <vector>
int main() {
    using xavrx::system::SdCardProtocol;
    int failures = 0;
    const auto check = [&](bool ok, const char* message) {
        if (!ok) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };
    SdCardProtocol card;
    check(!card.attach({}), "reject empty image");
    check(!card.attach(std::vector<std::uint8_t>(513U)), "reject partial block image");
    std::vector<std::uint8_t> image(4U * SdCardProtocol::block_size, 0U);
    image[0] = 0x5aU; image[512U] = 0xa5U;
    check(card.attach(std::move(image)) && card.present() && card.blocks() == 4U, "attach block image");
    auto r = card.command(0U, 0U);
    check(r.accepted && r.r1 == 1U && card.state() == SdCardProtocol::State::idle, "CMD0 resets to idle");
    r = card.command(8U, 0x1aaU);
    check(r.accepted && r.value == 0x1aaU, "CMD8 echoes voltage/check pattern");
    r = card.command(8U, 0x2aaU);
    check(!r.accepted && (r.r1 & 0x04U) != 0U && card.state() == SdCardProtocol::State::idle, "CMD8 rejects unsupported voltage without changing card state");
    r = card.command(8U, 0x10001aaU);
    check(!r.accepted && (r.r1 & 0x04U) != 0U, "CMD8 rejects nonzero reserved argument bits");
    r = card.command(8U, 0x1abU);
    check(r.accepted && r.value == 0x1abU, "CMD8 echoes a non-default check pattern");
    r = card.command(55U, 0U);
    check(r.accepted && r.r1 == 1U, "CMD55 arms application command");
    r = card.command(41U, 0x40000000U);
    check(!r.accepted && card.state() == SdCardProtocol::State::idle, "ACMD41 rejects an empty voltage window");
    r = card.command(55U, 0U);
    check(r.accepted, "CMD55 rearms after rejected ACMD41");
    r = card.command(41U, 0x00ff8000U);
    check(!r.accepted && card.state() == SdCardProtocol::State::idle, "SDHC ACMD41 requires host high-capacity support");
    r = card.command(55U, 0U);
    check(r.accepted, "CMD55 rearms for valid SDHC initialization");
    r = card.command(41U, 0x40ff8000U);
    check(r.accepted && r.r1 == 0U && card.state() == SdCardProtocol::State::ready, "ACMD41 enters ready state");
    r = card.command(2U, 0U);
    check(r.accepted && card.state() == SdCardProtocol::State::standby, "CMD2 returns CID and enters standby");
    r = card.command(3U, 0U);
    check(r.accepted && r.value == 0x12340000U, "CMD3 assigns RCA");
    r = card.command(7U, 0x12340000U);
    check(r.accepted && card.state() == SdCardProtocol::State::transfer, "CMD7 selects card");
    r = card.command(0U, 0U);
    check(r.accepted && card.state() == SdCardProtocol::State::idle, "CMD0 returns card to idle before invalid deselect check");
    r = card.command(7U, 0U);
    check(!r.accepted && card.state() == SdCardProtocol::State::idle && (r.r1 & 0x04U) != 0U, "CMD7 cannot deselect a card from idle state");
    r = card.command(55U, 0U);
    check(r.accepted, "CMD55 rearms application command after CMD0");
    r = card.command(41U, 0x40ff8000U);
    check(r.accepted, "ACMD41 restarts initialization after CMD0");
    r = card.command(2U, 0U);
    check(r.accepted, "CMD2 returns to standby after reinitialization");
    r = card.command(3U, 0U);
    check(r.accepted, "CMD3 assigns RCA after reinitialization");
    r = card.command(7U, 0x12340000U);
    check(r.accepted && card.state() == SdCardProtocol::State::transfer, "CMD7 reselects card after reinitialization");
    r = card.command(16U, 512U);
    check(r.accepted, "CMD16 sets 512-byte block length");
    r = card.command(17U, 0U);
    std::array<std::uint8_t,512> data{};
    check(r.accepted && card.read_data_block(data) && data[0] == 0x5aU, "CMD17 reads selected block");
    check(!card.read_data_block(data), "single-block read ends after one block");
    r = card.command(18U, 1U);
    check(r.accepted && card.read_data_block(data) && data[0] == 0xa5U, "CMD18 starts multi-block read");
    check(card.read_data_block(data), "CMD18 advances to following block");
    r = card.command(12U, 0U);
    check(r.accepted && !card.read_data_block(data), "CMD12 stops multi-block transfer");
    r = card.command(24U, 2U);
    data.fill(0x3cU);
    check(r.accepted && card.write_data_block(data), "CMD24 writes one block");
    check(!card.write_data_block(data), "single-block write ends after one block");
    r = card.command(25U, 2U);
    data.fill(0x77U);
    check(r.accepted && card.write_data_block(data) && card.write_data_block(data), "CMD25 writes sequential blocks");
    r = card.command(13U, 0U);
    check(r.accepted && r.value == 0U, "CMD13 reports transfer status");
    r = card.command(18U, 1U);
    check(r.accepted, "CMD18 starts transfer before deselect regression");
    r = card.command(7U, 0U);
    check(r.accepted && !card.read_data_block(data), "CMD7 deselect aborts active data transfer");
    r = card.command(0U, 0U);
    check(r.accepted && card.state() == SdCardProtocol::State::idle &&
          !card.read_data_block(data), "CMD0 aborts any active data transfer");
    r = card.command(16U, 1024U);
    check(!r.accepted && (r.r1 & 0x04U) != 0U, "reject unsupported block length");
    card.reset();
    check(card.state() == SdCardProtocol::State::idle && card.present(), "reset preserves card image");
    card.detach();
    check(!card.present(), "detach removes card");
    if (failures != 0) { std::cerr << failures << " SD protocol test(s) failed\n"; return 1; }
    std::cout << "All SD protocol tests passed\n";
    return 0;
}

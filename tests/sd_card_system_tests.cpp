#include "xavrx/system/wii_system.hpp"
#include <cstdint>
#include <iostream>
#include <vector>
int main() {
 using namespace xavrx::system;
 int failures=0;
 const auto check=[&](bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';++failures;}};
 WiiSystem system;
 check(system.initialize(),"system initializes");
 std::vector<std::uint8_t> image(4U*SdCardProtocol::block_size,0U);
 image[0]=0x41U; image[512U]=0x42U;
 check(system.attach_sd_card(std::move(image)),"system attaches SD card to host and protocol models");
 auto& card=system.sd_card_protocol();
 check(card.present()&&card.blocks()==4U&&system.sd_host().card_present(),"both SD layers expose attached card");
 auto r=card.command(0U,0U);
 check(r.accepted&&r.r1==1U,"integrated card accepts reset command");
 r=card.command(55U,0U);check(r.accepted,"integrated card accepts application-command prefix");
 r=card.command(41U,0x40ff8000U);check(r.accepted&&card.state()==SdCardProtocol::State::ready,"integrated card completes ACMD41 initialization");
 r=card.command(2U,0U);check(r.accepted,"integrated card returns CID");
 r=card.command(3U,0U);check(r.accepted,"integrated card assigns RCA");
 r=card.command(7U,0x12340000U);check(r.accepted,"integrated card enters transfer state");
 r=card.command(17U,0U);
 std::array<std::uint8_t,SdCardProtocol::block_size> block{};
 check(r.accepted&&card.read_data_block(block)&&block[0]==0x41U,"integrated protocol reads attached image");
 check(system.sd_host().read_blocks(1U,1U,block)&&block[0]==0x42U,"existing SD host block interface remains synchronized");
 system.reset();
 check(card.present()&&card.state()==SdCardProtocol::State::idle,"system reset preserves media but resets SD protocol state");
 system.detach_sd_card();
 check(!card.present()&&!system.sd_host().card_present(),"system detaches both SD layers");
 if(failures){std::cerr<<failures<<" integrated SD test(s) failed\n";return 1;}
 std::cout<<"All integrated SD system tests passed\n";return 0;
}

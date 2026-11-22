#include "xavrx/system/input_mapping.hpp"
#include <cmath>
#include <iostream>
namespace { int failures=0; void check(bool ok,const char* m){if(!ok){std::cerr<<"FAIL: "<<m<<'\n';++failures;}} }
int main(){
 using namespace xavrx::system;
 ControllerInputRouter router;
 check(router.add_binding({ControllerType::WiiRemote,InputControl::RemoteA,GameAction::Confirm}),"add Wii Remote mapping");
 check(router.add_binding({ControllerType::WiiRemote,InputControl::PointerVisible,GameAction::PointerVisible}),"add Wii Remote pointer visibility mapping");
 check(router.add_binding({ControllerType::Nunchuk,InputControl::NunchukStickX,GameAction::MoveX,1.0F,0.1F,false}),"add Nunchuk axis mapping");
 check(router.add_binding({ControllerType::Classic,InputControl::ClassicTriggerL,GameAction::Primary,1.0F,0.2F,true}),"add Classic trigger mapping");
 check(router.add_binding({ControllerType::ClassicPro,InputControl::ProRightY,GameAction::CameraY,1.0F,0.1F,false}),"add Pro stick mapping");
 check(router.add_binding({ControllerType::BalanceBoard,InputControl::BalanceWeightKg,GameAction::WeightKg,1.0F,0.0F,false}),"add board weight mapping");
 check(!router.add_binding({ControllerType::WiiRemote,InputControl::RemoteA,GameAction::Confirm,1.0F,-1.0F,true}),"reject negative deadzone");
 check(!router.add_binding({static_cast<ControllerType>(99),InputControl::RemoteA,GameAction::Confirm}),"reject invalid controller profile");
 WiiRemoteController remote; (void)remote.set_button(WiiRemoteButton::A,true);
 auto state=router.route(make_input_frame(remote.state()));
 check(state.pressed(GameAction::Confirm),"Wii Remote A routes to confirm");
 remote.set_pointer(0.25F,0.75F,true);
 state=router.route(make_input_frame(remote.state()));
 check(state.pressed(GameAction::PointerVisible),"Wii Remote pointer visibility routes as a digital action");
 check(!router.route(make_input_frame(WiiRemoteState{})).pressed(GameAction::PointerVisible),"hidden Wii Remote pointer is not reported visible");
 NunchukController nunchuk; nunchuk.set_stick(0.5F,-0.8F);
 state=router.route(make_input_frame(nunchuk.state()));
 check(std::fabs(state.value(GameAction::MoveX)-((0.5F-0.1F)/0.9F))<0.001F,"Nunchuk axis deadzone and normalization");
 ClassicController classic; classic.set_triggers(0.8F,0.0F);
 state=router.route(make_input_frame(classic.state()));
 check(state.pressed(GameAction::Primary),"Classic analog trigger routes as digital action");
 ClassicControllerPro pro; pro.set_sticks(0.0F,0.0F,0.0F,-0.6F);
 state=router.route(make_input_frame(pro.state()));
 check(state.value(GameAction::CameraY)<-0.5F,"Classic Pro right stick routes to camera");
 WiiBalanceBoard board;
 for(unsigned i=0;i<4U;++i) (void)board.set_corner_calibration(static_cast<BalanceBoardCorner>(i),0.0F,0.01F);
 board.set_sensor_readings(1000,1000,1000,1000);
 state=router.route(make_input_frame(board.state()));
 check(std::fabs(state.value(GameAction::WeightKg)-0.2666667F)<0.001F,"Balance Board weight is exposed as normalized game input");
 state=router.route(make_input_frame(WiiRemoteState{}));
 check(!state.pressed(GameAction::Confirm)&&state.value(GameAction::MoveX)==0.0F,"unmapped profile produces neutral state");
 if(failures){std::cerr<<failures<<" input mapping test(s) failed\n";return 1;}
 std::cout<<"All XAVRX-WII input mapping tests passed\n";return 0;
}

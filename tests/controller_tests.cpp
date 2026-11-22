#include "xavrx/system/controllers.hpp"
#include <cmath>
#include <iostream>
#include <limits>
namespace { int failures=0; void check(bool ok,const char* msg){if(!ok){std::cerr<<"FAIL: "<<msg<<'\n';++failures;}} }
int main(){
 using namespace xavrx::system;
 WiiRemoteController remote;
 check(remote.set_button(WiiRemoteButton::A,true)&&remote.button(WiiRemoteButton::A),"Wii Remote A button press");
 check(remote.set_button(WiiRemoteButton::A,false)&&!remote.button(WiiRemoteButton::A),"Wii Remote A button release");
 check(remote.set_button(WiiRemoteButton::Home,true)&&remote.button(WiiRemoteButton::Home),"Wii Remote Home button");
 remote.set_pointer(1.4F,-0.2F,true);
 check(remote.state().pointer.x==1.0F&&remote.state().pointer.y==0.0F&&remote.state().pointer.visible,"Wii Remote pointer clamps to screen bounds");
 remote.set_pointer(std::numeric_limits<float>::quiet_NaN(),0.4F,false);
 check(remote.state().pointer.x==0.5F&&!remote.state().pointer.visible,"Wii Remote pointer handles non-finite input");
 remote.set_acceleration(2.0F,-2.0F,0.5F);
 check(remote.state().acceleration.x==1.0F&&remote.state().acceleration.y==-1.0F&&remote.state().acceleration.z==0.5F,"Wii Remote acceleration clamps safely");
 remote.set_nunchuk_attached(true);check(remote.state().nunchuk_attached,"Wii Remote extension connection state");
 remote.reset();check(remote.state().buttons==0U&&!remote.state().nunchuk_attached&&!remote.state().pointer.visible,"Wii Remote reset");

 NunchukController nunchuk;
 check(nunchuk.set_button(NunchukButton::C,true)&&nunchuk.button(NunchukButton::C),"Nunchuk C button");
 check(nunchuk.set_button(NunchukButton::Z,true)&&nunchuk.button(NunchukButton::Z),"Nunchuk Z button");
 nunchuk.set_stick(-1.5F,0.75F);
 check(nunchuk.state().stick_x==-1.0F&&nunchuk.state().stick_y==0.75F,"Nunchuk analog stick range");
 nunchuk.set_acceleration(0.2F,0.3F,4.0F);
 check(nunchuk.state().acceleration.z==1.0F,"Nunchuk acceleration range");
 nunchuk.reset();check(nunchuk.state().buttons==0U&&nunchuk.state().stick_x==0.0F,"Nunchuk reset");

 ClassicController classic;
 check(classic.set_button(ClassicButton::ZL,true)&&classic.button(ClassicButton::ZL),"Classic Controller ZL button");
 check(classic.set_button(ClassicButton::Home,true)&&classic.button(ClassicButton::Home),"Classic Controller Home button");
 classic.set_sticks(-1.0F,0.5F,0.25F,1.5F);
 check(classic.state().left_x==-1.0F&&classic.state().left_y==0.5F&&classic.state().right_x==0.25F&&classic.state().right_y==1.0F,"Classic Controller dual analog sticks");
 classic.set_triggers(-0.5F,1.5F);
 check(classic.state().trigger_l==0.0F&&classic.state().trigger_r==1.0F,"Classic Controller analog trigger range");
 classic.reset();check(classic.state().buttons==0U&&classic.state().trigger_r==0.0F,"Classic Controller reset");
 if(failures){std::cerr<<failures<<" controller test(s) failed\n";return 1;}
 std::cout<<"All XAVRX-WII controller tests passed\n";return 0;
}

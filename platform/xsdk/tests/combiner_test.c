#include <assert.h>
#include "gfx_cc.h"
int main(void) {
 struct CombineMode cm = {0};
 cm.rgb1 = 0x03020100u; cm.alpha1 = 0x07060504u;
 cm.rgb2 = 0x0b0a0908u; cm.alpha2 = 0x0f0e0d0cu;
 for (unsigned i=0; i<16; ++i) assert(gfx_cc_get_command(&cm,i)==i);
 // Texture multiplied by shade: (texture - zero) * shade + zero.
 cm.rgb1 = CC_TEXEL0 | (CC_SHADE << 16);
 assert(gfx_cc_get_command(&cm,0)==CC_TEXEL0);
 assert(gfx_cc_get_command(&cm,1)==CC_0);
 assert(gfx_cc_get_command(&cm,2)==CC_SHADE);
 assert(gfx_cc_get_command(&cm,3)==CC_0);
 return 0;
}

#include <ZAP/token.hpp>

token::token(tokenType t, string c, int l, int lp){
    type = t;
    content = c;
    line = l;
    linePos = lp;
}
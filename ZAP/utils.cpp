#include <ZAP/utils.hpp>

bool isNumber(char input){
    return input >= '0' && input <= '9';
}
bool isCharacter(char input){
    return (input >= 'a' && input <= 'z') || (input >= 'A' && input <= 'Z');
}
bool isAlphanumeric(char input){
    return isNumber(input) || isCharacter(input);
}
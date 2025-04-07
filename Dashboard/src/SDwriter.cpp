#include "dashboard.h"

String pad(int number)
{
    return number < 10 ? "0" + String(number) : String(number);
}

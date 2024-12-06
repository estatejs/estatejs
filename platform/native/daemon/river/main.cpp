#include <estate/runtime/enum_op.h>
#include "estate/internal/river/river.h"

using namespace estate;

int main() {
    auto config = River::LoadConfig();
    River river{};
    river.init(config);
    river.run(true);
}

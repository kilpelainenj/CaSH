#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
extern "C" { 
#include "command.h" 
#include "simple_command.h"
}

TEST_CASE("cmd grows & clears [unit][command]") {
    Command cmd; cmd_init(&cmd);
    for (int i = 0; i <8; i++){auto* sc = static_cast<SimpleCommand*>(std::malloc(sizeof(SimpleCommand))); sc_init(sc); cmd_insert_sc(&cmd, sc);  }
    REQUIRE(cmd.size == 8);
    REQUIRE(cmd.capacity >= 8);

    cmd_clear(&cmd);
    REQUIRE(cmd.size == 0);
}
-- @Param test_list: List[Str[3]] -> [{"test_name", "test_file", "test_args"},..]
run_tests = function (bin_path, test_list)
    local ANSI_RES,ANSI_RED,ANSI_YEL = "\27[0m","\27[31m","\27[33m"
    local total_passed = 0
    local test_res = { }
    table.foreachi(table.keys(test_list), function(index)
        test_res[index] = os.execute(bin_path .. "/" .. test_list[index][1] .. " " .. test_list[index][3]) == true
    end)
    printf("\n---");
    table.foreachi(table.keys(test_list), function(index)
        local passed = test_res[index]
        printf("%s%s [%s].", (passed and ANSI_YEL.."OK  " or ANSI_RED.."FAIL"), ANSI_RES, test_list[index][1])
        if passed then total_passed = total_passed + 1 end
    end)
    printf("%s%d/%d Test passed.\n---", ANSI_RES, total_passed, #test_list);
    os.exit((#test_list == total_passed) and 0 or 1)   -- Return error on failure.
end

setup_tests = function(test_list)
    table.foreachi(test_list, function(test)
        project (test[1]) kind "ConsoleApp" language "C" uses "woycontainer" files { test[2] }
    end)
end

setup_fuzz_tests = function(test_list)
    table.foreachi(test_list, function(test)
        project (test[1]) kind "ConsoleApp" language "C" uses "woycontainer" files { test[2] }
        enable_fuzzer()
        defines { "FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION" }
    end)
end

-- CLANG only.
enable_fuzzer = function()
    sanitize { "Fuzzer" }
end

-- Same flags for GCC and CLANG.
enable_color_diagnostics = function()
    buildoptions { "-fdiagnostics-color=always" }
end

-- Same flags for GCC and CLANG.
enable_coverage = function()
    buildoptions { "-fprofile-arcs", "-ftest-coverage" } linkoptions { "--coverage" }
    --filter "toolset:gcc" buildoptions { "-fprofile-arcs", "-ftest-coverage" } linkoptions { "--coverage" }
    --filter "toolset:clang" buildoptions { "-fprofile-instr-generate", "-fcoverage-mapping" } linkoptions { "-fprofile-instr-generate", "-fcoverage-mapping" }
end

local MY_WARNINGS = {
    '-Wall',
    '-Wextra',
    '-Wpedantic',
    '-Werror',
    '-Wshadow',
    '-Wno-language-extension-token',

    -- Preferences.
    '-Wno-error=unused-parameter',
    '-Wno-error=unused-function',
    '-Wno-error=unused-variable',
    '-Wno-unused-function',
    '-Wno-gnu-statement-expression',
    '-Wno-gnu-zero-variadic-macro-arguments'
}

workspace "main"
    configurations "Debug"
    location "build"
    cdialect "gnu11"
    buildoptions { MY_WARNINGS }
    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"                                -- -g
        optimize "Debug"                            -- -Og
        omitframepointer "On"                       -- -fno-omit-frame-pointer
        sanitize { "Address", "UndefinedBehavior" } -- -fsanitize=address,undefined
        enable_color_diagnostics()
        enable_coverage()

-- Include the library.
include "../"
project "woycontainer"
    enable_coverage()

-- Tests

test_list = {
    { "arena_test"      , "../src/arena_test.c"      , "" },
    { "slot_test"       , "../src/slot_test.c"       , "" },
    { "strpool_test"    , "../src/strpool_test.c"    , "" },
    { "array_test"      , "../src/array_test.c"      , "" },
    { "strmap_test"     , "../src/strmap_test.c"     , "" },
    { "umapstr_test"    , "../src/umapstr_test.c"    , "" },
    { "list_simple_test", "../src/list_simple_test.c", "" },
    { "wmap_test"       , "../src/wmap_test.c"       , "" },
    { "wmapstr_test"    , "../src/wmapstr_test.c"    , "" },
    { "wmapstr_test2"   , "../src/wmapstr_test2.c"   , "" },
    { "slotpool_test"   , "../src/slotpool_test.c"   , "" },
}
setup_tests(test_list)

-- Fuzzy tests

fuzz_test_list = {
    { "strpool_test_fuzz"    , "../src/slotpool_test.c"   , "-runs=5000" },
    { "wmap_test_fuzz"       , "../src/wmap_test.c"       , "-runs=5000" },
    { "wmapstr_test2_fuzz"   , "../src/wmapstr_test2.c"   , "-runs=5000" },
    { "slotpool_test_fuzz"   , "../src/slotpool_test.c"   , "-runs=10000" },
}
setup_fuzz_tests(fuzz_test_list)


newaction {
    trigger = "test",
    description = "Run automatic tests.",
    execute = function () run_tests ("./build/bin/Debug/", test_list) end
}

newaction {
    trigger = "fuzztest",
    description = "Run automatic tests (CLANG ONLY).",
    execute = function () run_tests ("./build/bin/Debug/", fuzz_test_list) end
}

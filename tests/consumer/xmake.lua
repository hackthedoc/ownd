set_project("ownd_consumer")
set_version("0.1.0")
set_languages("cxx23")

add_rules("mode.debug", "mode.release")

set_warnings("allextra")

target("ownd_consumer")
    set_kind("binary")

    add_files("main.cpp")
    add_includedirs("../../build/install/include")
target_end()

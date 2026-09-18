set_project("ownd")
set_version("0.1.0")
set_languages("cxx23")

add_rules("mode.debug", "mode.release")
set_warnings("allextra")

option("tests")
    set_default(false)
    set_showmenu(true)
    set_description("Build the Ownd test suite")
option_end()

target("ownd")
    set_kind("headeronly")
    add_headerfiles("include/(ownd/**.hpp)")
    add_includedirs("include", {public = true})
target_end()

if has_config("tests") then
    target("ownd_tests")
        set_kind("binary")
        add_files("tests/**.cpp")
        add_deps("ownd")
        add_includedirs("vendors")
    target_end()
end

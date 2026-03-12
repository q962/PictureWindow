set_xmakever("3.0.5")

set_project("PictureWindow")
set_version("0.0.1")

APPID = "io.github.q962.PictureWindow"
APPRESPREFIX = "/io/github/q962/PictureWindow/"

set_config("APPID", APPID)
set_configvar("APPID", APPID)
set_configvar("APPRESPREFIX", APPRESPREFIX)

option("ENABLE_PROJECT_PREFIX", { default = false, description = "", defines = { "ENABLE_PROJECT_PREFIX", "PROJECT_PREFIX=\"$(projectdir)/res\"" } })

add_rules("mode.debug", "mode.release", "mode.releasedbg")

includes("xmake-modules/rules/gnome")

add_requireconfs("*", {
    system = true
})
add_requires("gtk4 >=4.20")
add_requires("glycin-gtk4-2", "librsvg-2.0")

target("PictureWindow")
do
    add_packages("gtk4", "glycin-gtk4-2", "librsvg-2.0")

    add_configfiles("src/config.h.in")

    add_rules("gnome")
    add_options("ENABLE_PROJECT_PREFIX")

    add_files("src/**.c")

    before_run(function(target)

        target:add("runenvs", "XDG_DATA_DIRS", target:scriptdir() .. "/res")
    end)
end

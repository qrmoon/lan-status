set_config("cflags", "-Wall -Wextra -Wpedantic")

if is_plat("linux") then
    add_requires("appindicator3-0.1", "gtk+-3.0")
end

target("client")
    set_kind("binary")
    add_files("src/client.c")
    if is_plat("linux") then
        add_packages("appindicator3-0.1", "gtk+-3.0")
        add_files("src/tray/tray_linux.c")
    elseif is_plat("windows", "mingw") then
        add_files("src/tray/tray_windows.c")
        add_links("ws2_32")
    end

target("server")
    set_kind("binary")
    add_files("src/server.c")
    if is_plat("linux") then
        add_packages("appindicator3-0.1", "gtk+-3.0")
        add_files("src/tray/tray_linux.c")
    elseif is_plat("windows", "mingw") then
        add_files("src/tray/tray_windows.c")
        add_links("ws2_32")
    end

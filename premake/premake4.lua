--Build script for Sparse Texture Voxels

--Application name
local name = "Sparse Texture Voxels"

--Directory structure
local cwd = os.getcwd() .. "/"
local project_location = "../"
local build_location =      project_location .. "build/"
local source_location =     project_location .. "src/"
local data_location =       project_location .. "data/"
local externals_location =  project_location .. "external/"

local lib_locations = {
    "glfw-2.7.6/lib/"
}
local header_locations = {
    "glfw-2.7.6/include/",
    "gl3w/include/",
    "glm-0.9.3.4/",
    "gli-0.3.0.3/",
    "tinyxml2-78d450b/"
}
local source_locations = {
    "tinyxml2-78d450b/tinyxml2.cpp",
    "gl3w/src/gl3w.c"
}

--Adds libs to the project
--Folder structure: "libName/lib/os_type/platform_type/build_type/file_name"
--Example: "glfw-2.7.6/lib/windows/x32/Release/GLFW.lib"
--_OPTIONS are the command line arguments when running premake.exe. Look at the batch script.
function addlibs(build_type)
    local os_type = _OPTIONS["os"]                      --Possible types: linux, macosx, windows, and more (http://industriousone.com/osis)
    local platform_type = _OPTIONS["platform"]          --Possible types: x32, x64, and more (http://industriousone.com/platforms-0)
    local build_type = build_type                       --Possible types: Release, Debug
    local endpath = os_type .. "/" .. platform_type .. "/" .. build_type .. "/"
    for i,lib in pairs(lib_locations) do
        local full_path = cwd .. externals_location .. lib .. endpath
        libdirs (full_path)
        local libs = os.matchfiles(full_path .. "*")
        for i=1, #libs do links (path.getbasename(libs[i])) end
    end
end

--Include header files
function addHeaders()
    for i,header in pairs(header_locations) do
        local full_path = cwd .. externals_location .. header
        includedirs(full_path);
    end
end

--Include external source files
function addSources()
    for i,source in pairs(source_locations) do
        local full_path = cwd .. externals_location .. source
        files (full_path)
    end
end

--Delete the old build folder
os.rmdir(cwd .. string.gsub(build_location, "/$", ""))

--Set up debug and release versions
solution ( name )
    configurations { "Debug", "Release" }
    location ( build_location )

    --Various variables defined here
    defines {"_CRT_SECURE_NO_WARNINGS"}

project ( name )
    kind ("ConsoleApp")
    language ("C++")
    addHeaders ()                               --add library headers
    addSources ()                               --add library sources
    files { source_location  .. "**" }          --include all of our source code (recursive)
    files { data_location    .. "**" }          --include all of the data files
    location ( build_location )                 --this is where the project is built
    debugdir ( project_location )               --this is where the IDE-generated-exe accesses data and other folders
    targetdir ( build_location )                --this is where the exe gets built
    targetextension ( ".exe" )                  --Windows executable type
    links ( "opengl32" )                        --finds the opengl lib file

    --Debug-----------------------------------
    configuration "Debug"
        flags   { "Symbols" }
        defines { "DEBUG" }
        addlibs ( "Debug" )
    --Release---------------------------------
    configuration "Release"
        flags   { "Optimize" }
        defines { "NDEBUG" }
        addlibs ( "Release" )

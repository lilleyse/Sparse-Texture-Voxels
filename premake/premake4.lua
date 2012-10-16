--Build script for Sparse Texture Voxels

--Application name
local name = "Sparse Texture Voxels"

--Directory structure
local cwd = os.getcwd() .. "/"
local project_location = "../"
local build_location =      project_location .. "build/"                --Build
local source_location =     project_location .. "src/"                  --Source
local data_location =       project_location .. "data/"                 --Data
local externals_location =  project_location .. "external/"             --External libraries and headers

local lib_locations = {"glew-1.9.0/lib/", "glfw-2.7.6/lib/"}
local header_locations = {"glew-1.9.0/include/", "glfw-2.7.6/include/", "glm-0.9.3.4/", "gli-0.3.0.3/", "tinyxml2-78d450b/"}
local tinyxmlcpp = externals_location .. "tinyxml2-78d450b/tinyxml2.cpp"

--Collects the lib names within a certain folder
function matchlibs(dir)
    local libs = os.matchfiles(dir .. "*")
    for i=1, #libs do
        libs[i] = string.gsub(libs[i], ".lib$", "")
        libs[i] = string.gsub(libs[i], dir, "")
    end
    return libs
end

--Adds libs to the project
--Folder structure: "libName/os_type/platform_type/build_type/file_name"
--Example: "SFML_2.0/windows/x32/Debug/sfml-audio-s-d.lib"
--_OPTIONS are the command line arguments when running premake.exe. Look at the batch script.
function addlibs(build_type)
    local os_type = _OPTIONS["os"]                      --Possible types: linux, macosx, windows, and more (http://industriousone.com/osis)
    local platform_type = _OPTIONS["platform"]          --Possible types: x32, x64, and more (http://industriousone.com/platforms-0)
    local build_type = build_type                       --Possible types: Release, Debug
    local endpath = os_type .. "/" .. platform_type .. "/" .. build_type .. "/"
    for i,lib in pairs(lib_locations) do
        local full_path = cwd .. externals_location .. lib .. endpath
        links (matchlibs(full_path))
        libdirs (full_path)
    end
end

--Include header files
function addHeaders()
    for i,header in pairs(header_locations) do
        local full_path = cwd .. externals_location .. header
        includedirs(full_path);
    end
end

--Delete the old build folder
os.rmdir(cwd .. string.gsub(build_location, "/$", ""))

--Set up debug and release versions
solution ( name )
    configurations { "Debug", "Release" }
    location ( build_location )

    --Various variables defined here
    defines {"_CRT_SECURE_NO_WARNINGS", "GLEW_STATIC"}

project ( name )
    kind ("ConsoleApp")
    language ("C++")
    files { source_location  .. "**" }          --include all of our source code (recursive)
    files { data_location    .. "**" }          --include all of the data files
    files { tinyxmlcpp }                        --include tinyxml (not sure how to build as library)
    addHeaders ()                               --this accounts for all library headers
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

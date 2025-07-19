require('scripts/generate_compile_commands')

workspace "smoljson"
   configurations { "Debug", "Release" }
   location "build"

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"

   filter {}
   
   targetdir "bin/%{cfg.buildcfg}"

   includedirs {
      "./include",
      "."
   }

project "testapp"
   kind "ConsoleApp"

   language "C++"
   cppdialect "C++17"

   files {
      "./src/testapp.cpp",
      "./include/**",
   }

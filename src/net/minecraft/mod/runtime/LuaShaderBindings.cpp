#include "net/minecraft/mod/runtime/LuaShaderBindings.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace gl = net::minecraft::client::gl;
namespace {
std::vector<unsigned int> g_activePrograms;
int luaShaderCreate(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.gettop(state) < 2) {
    api.pushinteger(state, -1);
    return 1;
  }
  const std::string vertSrc = luaString(state, 1, "");
  const std::string fragSrc = luaString(state, 2, "");
  gl::GLCore::ensureLoaded();
  if(!gl::GLCore::createShader || !gl::GLCore::createProgram) {
    std::cerr << "Shader compilation failed: OpenGL Shader extensions not supported/loaded." << std::endl;
    api.pushinteger(state, -1);
    return 1;
  }
  // Compile Vertex Shader
  unsigned int vs = gl::GLCore::createShader(0x8B31 /* GL_VERTEX_SHADER */);
  const char* vsCStr = vertSrc.c_str();
  gl::GLCore::shaderSource(vs, 1, &vsCStr, nullptr);
  gl::GLCore::compileShader(vs);
  int success = 0;
  gl::GLCore::getShaderiv(vs, 0x8B81 /* GL_COMPILE_STATUS */, &success);
  if(!success) {
    char log[1024];
    gl::GLCore::getShaderInfoLog(vs, sizeof(log), nullptr, log);
    std::cerr << "Vertex shader compilation failed:\n"
              << log << std::endl;
    gl::GLCore::deleteShader(vs);
    api.pushinteger(state, -1);
    return 1;
  }
  // Compile Fragment Shader
  unsigned int fs = gl::GLCore::createShader(0x8B30 /* GL_FRAGMENT_SHADER */);
  const char* fsCStr = fragSrc.c_str();
  gl::GLCore::shaderSource(fs, 1, &fsCStr, nullptr);
  gl::GLCore::compileShader(fs);
  gl::GLCore::getShaderiv(fs, 0x8B81 /* GL_COMPILE_STATUS */, &success);
  if(!success) {
    char log[1024];
    gl::GLCore::getShaderInfoLog(fs, sizeof(log), nullptr, log);
    std::cerr << "Fragment shader compilation failed:\n"
              << log << std::endl;
    gl::GLCore::deleteShader(vs);
    gl::GLCore::deleteShader(fs);
    api.pushinteger(state, -1);
    return 1;
  }
  // Link Program
  unsigned int program = gl::GLCore::createProgram();
  gl::GLCore::attachShader(program, vs);
  gl::GLCore::attachShader(program, fs);
  gl::GLCore::linkProgram(program);
  // Clean up shader objects
  gl::GLCore::deleteShader(vs);
  gl::GLCore::deleteShader(fs);
  gl::GLCore::getProgramiv(program, 0x8B82 /* GL_LINK_STATUS */, &success);
  if(!success) {
    char log[1024];
    gl::GLCore::getProgramInfoLog(program, sizeof(log), nullptr, log);
    std::cerr << "Shader program link failed:\n"
              << log << std::endl;
    gl::GLCore::deleteProgram(program);
    api.pushinteger(state, -1);
    return 1;
  }
  g_activePrograms.push_back(program);
  api.pushinteger(state, static_cast<long long>(program));
  return 1;
}
int luaShaderDestroy(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.gettop(state) < 1) {
    api.pushboolean(state, false);
    return 1;
  }
  unsigned int program = static_cast<unsigned int>(api.tointegerx(state, 1, nullptr));
  if(gl::GLCore::deleteProgram) {
    gl::GLCore::deleteProgram(program);
    auto it = std::find(g_activePrograms.begin(), g_activePrograms.end(), program);
    if(it != g_activePrograms.end()) {
      g_activePrograms.erase(it);
    }
    api.pushboolean(state, true);
  } else {
    api.pushboolean(state, false);
  }
  return 1;
}
int luaShaderBind(lua_State* state) {
  LuaApi& api = luaApi();
  unsigned int program = 0;
  if(api.gettop(state) >= 1 && api.type(state, 1) != kLuaTNil) {
    program = static_cast<unsigned int>(api.tointegerx(state, 1, nullptr));
  }
  if(gl::GLCore::useProgram) {
    gl::GLCore::useProgram(program);
    api.pushboolean(state, true);
  } else {
    api.pushboolean(state, false);
  }
  return 1;
}
int luaShaderUnbind(lua_State* state) {
  LuaApi& api = luaApi();
  if(gl::GLCore::useProgram) {
    gl::GLCore::useProgram(0);
    api.pushboolean(state, true);
  } else {
    api.pushboolean(state, false);
  }
  return 1;
}
int luaShaderUniformFloat(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.gettop(state) < 3) {
    api.pushboolean(state, false);
    return 1;
  }
  unsigned int program = static_cast<unsigned int>(api.tointegerx(state, 1, nullptr));
  const std::string name = luaString(state, 2, "");
  if(!gl::GLCore::getUniformLocation) {
    api.pushboolean(state, false);
    return 1;
  }
  int loc = gl::GLCore::getUniformLocation(program, name.c_str());
  if(loc == -1) {
    api.pushboolean(state, false);
    return 1;
  }
  int args = api.gettop(state) - 2;
  if(args == 1) {
    gl::GLCore::uniform1f(loc, static_cast<float>(api.tonumberx(state, 3, nullptr)));
  } else if(args == 2) {
    gl::GLCore::uniform2f(loc, static_cast<float>(api.tonumberx(state, 3, nullptr)), static_cast<float>(api.tonumberx(state, 4, nullptr)));
  } else if(args == 3) {
    gl::GLCore::uniform3f(loc, static_cast<float>(api.tonumberx(state, 3, nullptr)), static_cast<float>(api.tonumberx(state, 4, nullptr)), static_cast<float>(api.tonumberx(state, 5, nullptr)));
  } else if(args >= 4) {
    gl::GLCore::uniform4f(loc, static_cast<float>(api.tonumberx(state, 3, nullptr)), static_cast<float>(api.tonumberx(state, 4, nullptr)), static_cast<float>(api.tonumberx(state, 5, nullptr)), static_cast<float>(api.tonumberx(state, 6, nullptr)));
  }
  api.pushboolean(state, true);
  return 1;
}
int luaShaderUniformInt(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.gettop(state) < 3) {
    api.pushboolean(state, false);
    return 1;
  }
  unsigned int program = static_cast<unsigned int>(api.tointegerx(state, 1, nullptr));
  const std::string name = luaString(state, 2, "");
  if(!gl::GLCore::getUniformLocation) {
    api.pushboolean(state, false);
    return 1;
  }
  int loc = gl::GLCore::getUniformLocation(program, name.c_str());
  if(loc == -1) {
    api.pushboolean(state, false);
    return 1;
  }
  int args = api.gettop(state) - 2;
  if(args == 1) {
    gl::GLCore::uniform1i(loc, static_cast<int>(api.tointegerx(state, 3, nullptr)));
  } else if(args == 2) {
    gl::GLCore::uniform2i(loc, static_cast<int>(api.tointegerx(state, 3, nullptr)), static_cast<int>(api.tointegerx(state, 4, nullptr)));
  } else if(args == 3) {
    gl::GLCore::uniform3i(loc, static_cast<int>(api.tointegerx(state, 3, nullptr)), static_cast<int>(api.tointegerx(state, 4, nullptr)), static_cast<int>(api.tointegerx(state, 5, nullptr)));
  } else if(args >= 4) {
    gl::GLCore::uniform4i(loc, static_cast<int>(api.tointegerx(state, 3, nullptr)), static_cast<int>(api.tointegerx(state, 4, nullptr)), static_cast<int>(api.tointegerx(state, 5, nullptr)), static_cast<int>(api.tointegerx(state, 6, nullptr)));
  }
  api.pushboolean(state, true);
  return 1;
}
} // namespace
void installShaderApi(lua_State* state) {
  LuaApi& api = luaApi();
  pushFunctionTable(state,
                    {
                        {"create", luaShaderCreate},
                        {"destroy", luaShaderDestroy},
                        {"bind", luaShaderBind},
                        {"unbind", luaShaderUnbind},
                        {"uniform_float", luaShaderUniformFloat},
                        {"uniform_int", luaShaderUniformInt},
                    });
  api.setfield(state, -2, "shader");
}
} // namespace net::minecraft::mod::runtime

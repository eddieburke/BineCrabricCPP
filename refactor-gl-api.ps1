# Migrate GL11 pseudo-class call sites to unified gl:: free-function API in GlState.hpp.
$ErrorActionPreference = "Stop"
$root = Join-Path $PSScriptRoot "src"
$files = Get-ChildItem -Path $root -Recurse -Include *.cpp,*.hpp | Where-Object {
  $_.FullName -notmatch "\\build-omega\\"
}

$fnMap = @(
  "glEnableClientState", "glDisableClientState", "glCopyTexSubImage2D", "glLightModelfv", "glTexSubImage2D",
  "glGetIntegerv", "glGetFloatv", "glTexParameteri", "glTexCoordPointer", "glPolygonOffset", "glActiveTexture",
  "glColorMaterial", "glEnable", "glDisable", "glBlendFunc", "glColor4f", "glColor3f", "glColor4ub", "glShadeModel",
  "glDepthMask", "glColorMask", "glPushMatrix", "glPopMatrix", "glLoadIdentity", "glMatrixMode", "glTranslatef",
  "glScalef", "glRotatef", "glBindTexture", "glGenTextures", "glTexImage2D", "glDeleteTextures", "glAlphaFunc",
  "glClear", "glClearDepth", "glClearColor", "glViewport", "glScissor", "glDrawBuffer", "glReadBuffer", "glOrtho",
  "glBegin", "glEnd", "glTexCoord2d", "glVertex3d", "glNormal3b", "glScaled", "glNormal3f", "glFogi", "glFogf",
  "glFogfv", "glLightfv", "glVertexPointer", "glColorPointer", "glNormalPointer", "glDrawArrays", "glLineWidth",
  "glPointSize", "glDepthFunc", "glCullFace", "glGetError"
)

function GlFnToApi([string]$name) {
  if($name.StartsWith("gl") -and $name.Length -gt 2) {
    $rest = $name.Substring(2)
    return $rest.Substring(0, 1).ToLowerInvariant() + $rest.Substring(1)
  }
  return $name
}

foreach($file in $files) {
  $text = [IO.File]::ReadAllText($file.FullName)
  $orig = $text

  $text = $text -replace '#include "net/minecraft/client/gl/GL11.hpp"\r?\n', "#include `"net/minecraft/client/gl/GlState.hpp`"`r`n"
  $text = $text -replace 'using net::minecraft::client::gl::GL11;\r?\n', ''

  foreach($fn in $fnMap) {
    $api = GlFnToApi $fn
    $text = $text -replace "gl::GL11::$fn", "gl::$api"
    $text = $text -replace "GL11::$fn", "gl::$api"
  }

  $text = $text -replace 'gl::GL11::GL_', 'gl::GL_'
  $text = $text -replace 'GL11::GL_', 'gl::GL_'
  $text = $text -replace 'GL11::s_matrixStack', 'gl::matrix::active'

  if($text -ne $orig) {
    [IO.File]::WriteAllText($file.FullName, $text)
    Write-Host "Updated $($file.FullName)"
  }
}

Write-Host "GL API refactor complete."

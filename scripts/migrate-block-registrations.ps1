$ErrorActionPreference = "Stop"
$BlockDir = Join-Path $PSScriptRoot "..\src\net\minecraft\block"

function Get-RegistrationSnippet {
    param(
        [string]$FunctionName,
        [int]$Priority,
        [string]$Body
    )
    return @"

namespace {

void $FunctionName()
{
$Body
}

MINECRAFT_REGISTER_BLOCK($FunctionName, $Priority);

} // namespace
"@
}

function Add-RegistrationToFile {
    param(
        [string]$RelativePath,
        [string]$Snippet,
        [string[]]$ExtraIncludes = @()
    )
    $path = Join-Path $BlockDir $RelativePath
    if (-not (Test-Path $path)) {
        throw "Missing block source file: $RelativePath"
    }
    $content = Get-Content -LiteralPath $path -Raw
    if ($content -match 'MINECRAFT_REGISTER_BLOCK') {
        return
    }
    foreach ($include in $ExtraIncludes) {
        if ($content -notmatch [regex]::Escape($include)) {
            $content = $content -replace '(#include[^\r\n]+[\r\n]+)', "`$1#include `"$include`"`r`n", 1
        }
    }
    if ($content -notmatch 'BlockAutoRegistry\.hpp') {
        $content = "#include `"net/minecraft/block/BlockAutoRegistry.hpp`"`r`n" + $content
    }
    if ($content -match '}\s*//\s*namespace net::minecraft::block\s*$') {
        $content = $content -replace '}\s*//\s*namespace net::minecraft::block\s*$', ($Snippet + "`r`n} // namespace net::minecraft::block`r`n")
    }
    elseif ($content -match '}\s*// namespace net::minecraft::block\s*$') {
        $content = $content -replace '}\s*// namespace net::minecraft::block\s*$', ($Snippet + "`r`n} // namespace net::minecraft::block`r`n")
    }
    else {
        $content = $content.TrimEnd() + "`r`n" + $Snippet + "`r`n} // namespace net::minecraft::block`r`n"
    }
    Set-Content -LiteralPath $path -Value $content -NoNewline
}

function Write-NewRegistrationFile {
    param(
        [string]$RelativePath,
        [string[]]$Includes,
        [string]$Snippet
    )
    $path = Join-Path $BlockDir $RelativePath
    $lines = @()
    foreach ($include in $Includes) {
        $lines += "#include `"$include`""
    }
    $lines += ""
    $lines += "namespace net::minecraft::block {"
    $lines += $Snippet.TrimStart("`r", "`n")
    $lines += "} // namespace net::minecraft::block"
    $lines += ""
    Set-Content -LiteralPath $path -Value ($lines -join "`r`n") -NoNewline
}

$mat = 'mat::Material'

$newFiles = @{
    "StoneBlock.cpp" = @{
        Priority = 1
        Includes = @(
            "net/minecraft/block/StoneBlock.hpp",
            "net/minecraft/block/BlockAutoRegistry.hpp",
            "net/minecraft/block/VanillaBlockSounds.hpp"
        )
        Body = @"
    (new StoneBlock(1, 1))->setHardness(1.5f)->setResistance(10.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("stone");
"@
        Function = "registerStoneBlock"
    }
    "CobblestoneBlock.cpp" = @{
        Priority = 4
        Includes = @(
            "net/minecraft/block/Block.hpp",
            "net/minecraft/block/BlockAutoRegistry.hpp",
            "net/minecraft/block/VanillaBlockSounds.hpp",
            "net/minecraft/block/material/Material.hpp"
        )
        Body = @"
    namespace mat = material;
    (new Block(4, 16, mat::$mat::STONE))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("stonebrick");
"@
        Function = "registerCobblestoneBlock"
    }
}

# Fix cobblestone - $mat variable issue in hashtable
$newFiles["CobblestoneBlock.cpp"].Body = @"
    namespace mat = material;
    (new Block(4, 16, mat::Material::STONE))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("stonebrick");
"@

Write-Host "Script scaffold - use generated inc file"

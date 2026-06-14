$ErrorActionPreference = "Stop"

# This script lives inside cevg/ directory
# Run from the parent directory: powershell -File cevg\pack.ps1
$projectDir = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) ""
$outFile = Join-Path $projectDir "cevg.tar.xz"
$tarFile = Join-Path $projectDir "cevg.tar"
$sevenZip = "C:\Program Files\7-Zip\7z.exe"

Write-Host "=== cevg pack script ==="
Write-Host "Project: $projectDir"
Write-Host "Output:  $outFile"
Write-Host ""

# Remove old artifacts
Remove-Item $outFile -Force -ErrorAction SilentlyContinue
Remove-Item $tarFile -Force -ErrorAction SilentlyContinue

# Build exclusion list file for 7z (more reliable than -xr! wildcards)
$excludeFile = Join-Path $projectDir "_pack_exclude.txt"
@"
.git\
build\
build-cpu\
build-cpu2\
bninja\
build_vk_on2\
*.tar
*.tar.xz
*.tar.gz
*.zip
*.tmp
_pack_exclude.txt
pack.ps1
"@ | Set-Content $excludeFile -Encoding UTF8

Write-Host "[1/2] Creating tar..."
& $sevenZip a -ttar $tarFile ".\" "-xr@${excludeFile}"
if ($LASTEXITCODE -ne 0) { Write-Host "TAR failed"; exit 1 }

Write-Host "[2/2] Compressing to xz (dict=128m, mx9)..."
& $sevenZip a -txz -mx9 -m0=lzma2:d=128m $outFile $tarFile
if ($LASTEXITCODE -ne 0) { Write-Host "XZ failed"; exit 1 }

# Cleanup
Remove-Item $tarFile -Force
Remove-Item $excludeFile -Force

$size = (Get-Item $outFile).Length
$sizeMB = [math]::Round($size / 1MB, 1)
Write-Host ""
Write-Host "Done: $outFile ($sizeMB MB)"

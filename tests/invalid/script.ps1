$dir = "9_Lexical_And_Syntax"
$outFile = "bundled_$dir.txt"
if (Test-Path $outFile) { Remove-Item $outFile }
Get-ChildItem -Path $dir -Filter "*.mox" | ForEach-Object {
    "`n// ==========================================`n// File: $dir\$($_.Name)`n// ==========================================`n" | Add-Content $outFile
    Get-Content $_.FullName | Add-Content $outFile
}
Write-Host "Created $outFile"

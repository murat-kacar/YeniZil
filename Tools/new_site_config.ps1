param([switch]$Force)  # -Force: var olan site_config.h'nin üzerine yazar (anahtar değişir, tüm üniteler yeniden yüklenmeli)

$root    = Split-Path -Parent $PSScriptRoot                               # Proje kök klasörü
$example = Join-Path $root 'Common/config/site_config.example.h'          # Şablon
$target  = Join-Path $root 'Common/config/site_config.h'                  # Üretilecek dosya, git dışı

if ((Test-Path $target) -and -not $Force) { Write-Error 'site_config.h zaten var. Üzerine yazmak için -Force kullanın; anahtar değişir, tüm üniteler yeniden yüklenmeli.'; exit 1 }

$random = [byte[]]::new(20)                                               # 4 bayt apartman kimliği + 16 bayt anahtar
[System.Security.Cryptography.RandomNumberGenerator]::Fill($random)       # Kriptografik rastgele üreteç

$id  = '0x{0:X8}' -f [BitConverter]::ToUInt32($random, 0)                 # Apartman kimliği
if ($id -eq '0x00000000') { $id = '0x00000001' }                          # 0 atanmamış demek, kullanılmaz
$key = ($random[4..19] | ForEach-Object { '0x{0:X2}' -f $_ }) -join ', ' # Anahtar baytları

$content = Get-Content -Path $example -Raw
$content = $content -replace 'kApartmentId = 0x00000000', "kApartmentId = $id"
$content = $content -replace '(kApartmentKey\[16\] = \{)[^}]*(\})', "`${1}$key`${2}"
[System.IO.File]::WriteAllText($target, $content, [System.Text.UTF8Encoding]::new($false))  # BOM'suz UTF-8

Write-Output "site_config.h üretildi: $target"
Write-Output 'Sıradaki adım: kNodeMacs tablosuna kartların gerçek MAC adreslerini yazın.'

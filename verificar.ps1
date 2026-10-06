param([string] $Executable = 'dist/DoceHamburgueria-Windows.exe')
$ErrorActionPreference = 'Stop'
$app = Join-Path $PSScriptRoot $Executable
$folder = Join-Path $PSScriptRoot ('test-data/' + [guid]::NewGuid().ToString())
function Invoke-Terminal([string] $text) {
    $start = New-Object System.Diagnostics.ProcessStartInfo
    $start.FileName = $app
    $start.Arguments = '--dados "' + $folder + '"'
    $start.UseShellExecute = $false
    $start.RedirectStandardInput = $true
    $start.RedirectStandardOutput = $true
    $start.StandardOutputEncoding = New-Object System.Text.UTF8Encoding
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $start
    [void]$process.Start()
    $process.StandardInput.WriteLine($text)
    $process.StandardInput.Close()
    $result = $process.StandardOutput.ReadToEnd()
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) { throw 'Execucao falhou.' }
    $process.Dispose()
    return $result
}
$flow = @('1', 'Saldo inicial', '100', 's', '', '2', 'Ingredientes; "pao"', '25.25', 's', '', '1', 'Valor invalido', '1,234', '', '3', '', '4', '', '0') -join "`n"
$output = Invoke-Terminal $flow
if (($output -join "`n") -notmatch 'Saldo atual:  R\$ 74,75') { throw 'Saldo incorreto.' }
$rows = @(Get-Content -LiteralPath (Join-Path $folder 'movimentacoes.dat'))
if ($rows.Count -ne 2) { throw 'Validacao dos registros falhou.' }
$reopened = Invoke-Terminal '0'
if (($reopened -join "`n") -notmatch 'Saldo atual:  R\$ 74,75') { throw 'Persistencia falhou.' }
$csv = Get-Content -LiteralPath (Join-Path $folder 'extrato.csv') -Raw
$csvBytes = [System.IO.File]::ReadAllBytes((Join-Path $folder 'extrato.csv'))
if ($csvBytes[0] -ne 255 -or $csvBytes[1] -ne 254) { throw 'CSV sem identificacao UTF-16 LE.' }
if (!$csv.StartsWith("sep=;`r`n")) { throw 'Separador do Excel ausente.' }
$csvRows = @(($csv -replace '^sep=;\r\n', '') | ConvertFrom-Csv -Delimiter ';')
if ($csvRows.Count -ne 2 -or $csvRows[0].'Valor (R$)' -ne '="100,00"' -or $csvRows[1].'Valor (R$)' -ne '="25,25"' -or $csvRows[1].'Saldo (R$)' -ne '="74,75"') { throw 'Colunas da exportacao incorretas.' }
if (@($csvRows[1].PSObject.Properties).Count -ne 5 -or @($csvRows[1].PSObject.Properties)[2].Value -ne 'Ingredientes; "pao"') { throw 'Escape da descricao incorreto.' }
$descriptionTitle = 'Descri' + [char]0x00e7 + [char]0x00e3 + 'o'
if (@($csvRows[0].PSObject.Properties)[2].Name -cne $descriptionTitle) { throw 'Acentos do cabecalho incorretos.' }
[xml]$spreadsheet = Get-Content -LiteralPath (Join-Path $folder 'extrato.xml') -Raw -Encoding UTF8
$ns = New-Object System.Xml.XmlNamespaceManager($spreadsheet.NameTable)
$ns.AddNamespace('ss', 'urn:schemas-microsoft-com:office:spreadsheet')
$columns = $spreadsheet.SelectNodes('//ss:Column', $ns)
$xmlRows = $spreadsheet.SelectNodes('//ss:Row', $ns)
if ($columns.Count -ne 5 -or $columns[2].GetAttribute('Width', $ns.LookupNamespace('ss')) -ne '300' -or $xmlRows.Count -ne 3) { throw 'Largura ou registros da planilha incorretos.' }
if ($xmlRows[0].ChildNodes[2].InnerText -cne $descriptionTitle -or $xmlRows[2].ChildNodes[2].InnerText -cne 'Ingredientes; "pao"') { throw 'Texto da planilha incorreto.' }
$negative = Invoke-Terminal (@('2', 'Saida cancelada', '100', 'n', '0') -join "`n")
if (@(Get-Content -LiteralPath (Join-Path $folder 'movimentacoes.dat')).Count -ne 2) { throw 'Cancelamento falhou.' }
$originalHistory = Get-Content -LiteralPath (Join-Path $folder 'movimentacoes.dat') -Raw
$originalCsv = [System.IO.File]::ReadAllText((Join-Path $folder 'extrato.csv'))
$originalXml = [System.IO.File]::ReadAllText((Join-Path $folder 'extrato.xml'))
$cancelled = Invoke-Terminal (@('5', 'n', '', '0') -join "`n")
if ($cancelled -match 'Senha:' -or (Get-Content -LiteralPath (Join-Path $folder 'movimentacoes.dat') -Raw) -cne $originalHistory) { throw 'Cancelamento da exclusao falhou.' }
$wrongPassword = Invoke-Terminal (@('5', 's', 'errada', '', '0') -join "`n")
if ($wrongPassword -notmatch 'Senha incorreta' -or (Get-Content -LiteralPath (Join-Path $folder 'movimentacoes.dat') -Raw) -cne $originalHistory) { throw 'Senha incorreta alterou o historico.' }
if ([System.IO.File]::ReadAllText((Join-Path $folder 'extrato.csv')) -cne $originalCsv -or [System.IO.File]::ReadAllText((Join-Path $folder 'extrato.xml')) -cne $originalXml) { throw 'Exclusao cancelada alterou os extratos.' }
$deleted = Invoke-Terminal (@('5', 's', '1234', '', '3', '', '0') -join "`n")
if ($deleted -notmatch 'Todo o historico foi deletado' -or $deleted -notmatch 'Nenhuma movimentacao registrada' -or (Get-Item -LiteralPath (Join-Path $folder 'movimentacoes.dat')).Length -ne 0) { throw 'Exclusao do historico falhou.' }
if ((Test-Path -LiteralPath (Join-Path $folder 'extrato.csv')) -or (Test-Path -LiteralPath (Join-Path $folder 'extrato.xml'))) { throw 'Extratos antigos nao foram excluidos.' }
$emptyReopened = Invoke-Terminal '0'
if ($emptyReopened -notmatch 'Saldo atual:  R\$ 0,00') { throw 'Exclusao nao persistiu.' }
$newEntry = Invoke-Terminal (@('1', 'Novo saldo', '10', 's', '', '0') -join "`n")
if ($newEntry -notmatch 'Saldo atual:  R\$ 10,00' -or @(Get-Content -LiteralPath (Join-Path $folder 'movimentacoes.dat')).Count -ne 1) { throw 'Registro apos exclusao falhou.' }
$deletedWithoutExports = Invoke-Terminal (@('5', 's', '1234', '', '0') -join "`n")
if ($deletedWithoutExports -match 'nao foi possivel excluir') { throw 'Exclusao sem extratos existentes falhou.' }
Write-Output 'OK: entradas, saidas, saldo, valor invalido, persistencia, CSV, cancelamento e exclusao protegida do historico.'

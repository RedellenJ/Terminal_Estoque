# Doc'ê Hamburgueria — Gestão financeira

Sistema C++ com interface de terminal, entradas, saídas, saldo, extrato com data e exportação CSV. Os valores são calculados em centavos inteiros. As movimentações são salvas automaticamente após confirmação.

Entradas aparecem em verde e saídas em vermelho no resumo, no menu, na confirmação do valor e no extrato do terminal. O saldo do resumo fica vermelho quando negativo e verde nos demais casos. A planilha `extrato.xml` também destaca entradas em verde e saídas em vermelho; CSV não armazena cores.

## Executar

Windows x64: abra `dist/DoceHamburgueria-Windows.exe`.

Linux x64:

```sh
chmod +x dist/DoceHamburgueria-Linux
./dist/DoceHamburgueria-Linux
```

Registre o dinheiro que já existe na conta como uma entrada com descrição “Saldo inicial”. Informe valores como `150,50` ou `150.50`, sem separadores de milhar. Saídas que deixam o saldo negativo exigem confirmação adicional.

Os dados ficam em `movimentacoes.dat`, ao lado do executável. O CSV fica em `extrato.csv` no mesmo local. Preserve esse arquivo de dados e faça cópias de segurança. Abra apenas uma instância por arquivo de dados. Para escolher outra pasta, execute com `--dados caminho/da/pasta`.

Este é um controle manual: não se conecta ao banco. O saldo corresponde às movimentações registradas. A exportação substitui o CSV anterior.

A opção `[4] Exportar CSV / planilha Excel` gera `extrato.csv` em UTF-16 LE com identificação de codificação para preservar os acentos no Excel, e `extrato.xml`, uma planilha que abre no Excel com cabeçalho destacado, colunas dimensionadas e quebra de linha nas descrições. Abra `extrato.xml` para usar a formatação; você pode salvá-lo como `.xlsx` pelo Excel. Ambos são gerados ao lado dos dados e substituídos na próxima exportação. Feche os arquivos no Excel antes de exportar novamente.

O CSV inclui a indicação `sep=;` para abrir as cinco colunas corretamente. Valor e saldo são números com duas casas decimais. CSV não armazena largura de colunas: para ajustá-las no Excel, selecione A até E e dê dois cliques na divisão entre os cabeçalhos. Ao importar em outros programas, selecione UTF-16 LE, ignore a primeira linha `sep=;` e use o delimitador ponto e vírgula.

A opção `[5] Deletar todo o historico` apaga todas as movimentações e zera o saldo. O aviso “Você tem certeza que deseja deletar? Esse ato não pode ser revertido.” aparece em vermelho no terminal. Para apagar, digite `s` e depois a senha provisória `1234`. Qualquer outra resposta ou senha cancela a exclusão. O histórico apagado permanece vazio ao reabrir o programa. A exclusão também remove `extrato.csv` e `extrato.xml` da pasta dos dados. Se um deles estiver aberto no Excel, o programa informa que não conseguiu removê-lo; feche o arquivo e repita a exclusão.

## Compilar novamente

Com Zig 0.13.0 extraído em `tools/zig-windows-x86_64-0.13.0`, execute `./compilar.ps1` no PowerShell. No Linux com g++: `g++ -std=c++17 -O2 src/main.cpp -o DoceHamburgueria`.

O executável Linux usa musl com vinculação estática, sem precisar instalar bibliotecas C++.

# Doc'ê Hamburgueria — Gestão financeira com Supabase

Aplicação de terminal em C++17 para registrar entradas e saídas, consultar saldo e extrato e exportar relatórios.

As movimentações são armazenadas exclusivamente no banco remoto do Supabase. O programa requer conexão com a internet e não utiliza mais `movimentacoes.dat`, armazenamento local alternativo ou fila offline. Os valores são tratados em centavos inteiros, e o saldo corresponde às movimentações registradas manualmente. O sistema não acessa a conta bancária da hamburgueria.

## Configuração e acesso

A execução depende de um projeto Supabase configurado, um usuário do Supabase Authentication e das funções RPC `finance_load`, `finance_add` e `finance_clear`. As definições SQL dessas funções e das políticas de acesso não estão incluídas neste repositório; compilar a aplicação não configura o servidor nem importa históricos antigos.

O auxiliar `supabase.ps1` lê o `.env` localizado ao lado dele. Variáveis de ambiente já definidas têm prioridade. Configure:

```dotenv
SUPABASE_URL=https://seu-projeto.supabase.co
SUPABASE_PUBLISHABLE_KEY=sua-chave-publicavel
SUPABASE_EMAIL=seu-email
```

Use a chave publicável do projeto ou uma chave legada com papel `anon`. A senha é solicitada ao iniciar e também pode ser fornecida pela variável de ambiente `SUPABASE_PASSWORD`. Não é necessário salvá-la no `.env`.

O `.env` é ignorado pelo Git e copiado da raiz para `dist/.env` durante a compilação. Preserve as credenciais e compartilhe a configuração apenas com quem deve acessar o caixa.

O histórico acessível depende da conta autenticada e das regras do servidor. Para acessar o mesmo caixa em computadores diferentes, utilize a mesma conta. A aplicação envia as operações pelas RPCs autenticadas; o isolamento entre contas e a validação de gravações dependem da configuração do Supabase.

## Executar no Windows

Mantenha estes arquivos juntos em `dist/`:

- `DoceHamburgueria-Windows.exe`;
- `supabase.ps1`;
- `.env` com a configuração de acesso.

Abra o executável em um terminal e informe a senha, ou execute na raiz do projeto:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\iniciar.ps1
```

A entrada da senha fica oculta. O auxiliar utiliza o Windows PowerShell disponível no sistema. Para instalar em outro computador, leve o executável Windows, o auxiliar e o `.env` configurado.

## Executar no Linux

Instale PowerShell 7, com o comando `pwsh` disponível no ambiente. Mantenha `supabase.ps1` e `.env` ao lado de `DoceHamburgueria-Linux` e execute em um terminal:

```sh
chmod +x dist/DoceHamburgueria-Linux
./dist/DoceHamburgueria-Linux
```

O programa solicita a senha sem exibi-la quando `SUPABASE_PASSWORD` não está definida. A versão Linux gerada por `compilar.ps1` usa musl com vinculação estática das bibliotecas C++; o PowerShell continua sendo uma dependência externa.

## Uso

| Opção | Ação |
| --- | --- |
| `[1]` | Registrar entrada ou saldo inicial |
| `[2]` | Registrar saída |
| `[3]` | Consultar extrato completo |
| `[4]` | Exportar CSV e planilha XML para Excel |
| `[5]` | Excluir todo o histórico da conta conectada |
| `[0]` | Sair |

Registre o dinheiro inicial como uma entrada com descrição “Saldo inicial”, se ainda não estiver registrado no histórico remoto. Informe valores positivos como `150,50` ou `150.50`, com até duas casas decimais e sem separadores de milhar. A descrição aceita até 200 bytes e não aceita tabulações ou quebras de linha; uma descrição vazia cancela o cadastro.

Entradas e saídas exigem confirmação com `s`. Uma saída superior ao saldo consultado exige também confirmação de saldo negativo. Essa autorização é enviada ao servidor junto com a movimentação. O cliente limita cada valor e os totais de entradas e saídas a R$ 10.000.000.000,00.

O programa consulta o banco a cada retorno ao menu. Se uma operação de acesso falhar, ele informa o erro e encerra. Se a conexão cair durante uma gravação, confira o extrato ao reabrir antes de repetir: o servidor pode ter confirmado a movimentação mesmo que a resposta não tenha chegado ao programa.

## Exportação de relatórios

A opção `[4]` consulta o histórico remoto e gera, somente por solicitação:

- `extrato.csv`: UTF-16 LE com BOM, separador `;` e primeira linha `sep=;`. Valor e saldo são escritos como fórmulas de texto para o Excel preservar as duas casas decimais.
- `extrato.xml`: planilha SpreadsheetML com cores para entradas e saídas, colunas dimensionadas, quebra de linha nas descrições e células numéricas para valor e saldo. Pode ser aberta no Excel e salva como `.xlsx`.

Ambos contêm data, tipo, descrição, valor e saldo acumulado. A próxima exportação substitui os arquivos anteriores. Feche os relatórios no Excel antes de exportar novamente ou excluir o histórico. Ao importar o CSV em outros programas, selecione UTF-16 LE, ignore a linha `sep=;` e considere o formato das células de valor e saldo descrito acima.

Por padrão, os relatórios ficam ao lado do executável. Para escolher outra pasta:

```powershell
.\dist\DoceHamburgueria-Windows.exe --dados "C:\Relatorios"
```

```sh
./dist/DoceHamburgueria-Linux --dados ./relatorios
```

O argumento `--dados` define apenas o destino dos relatórios. As movimentações continuam no Supabase, e o auxiliar e a configuração continuam sendo procurados ao lado do executável. Os relatórios não são usados para restaurar ou carregar o histórico.

## Exclusão do histórico

A opção `[5]` exige duas confirmações: `s` e depois `APAGAR`. Qualquer outra resposta cancela a operação. A antiga senha fixa `1234` não é utilizada.

Após a RPC `finance_clear` concluir, o programa limpa o histórico em memória e tenta remover `extrato.csv` e `extrato.xml` da pasta de relatórios selecionada. Se um relatório não puder ser removido, uma mensagem informa a falha; isso não desfaz a exclusão no servidor. Cópias dos relatórios em outras pastas ou computadores permanecem existentes.

A exclusão do histórico remoto é irreversível pela aplicação e afeta o histórico acessível pela conta conectada, inclusive ao acessá-la em outro computador.

## Compilação

No Windows, extraia Zig 0.13.0 em `tools/zig-windows-x86_64-0.13.0` e execute na raiz:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\compilar.ps1
```

O script compila `src/main.cpp` com C++17 para Windows e Linux, gera os executáveis em `dist/`, copia `src/supabase.ps1` e, quando presente, copia o `.env` da raiz.

Como alternativa, no Linux com g++:

```sh
g++ -std=c++17 -O2 src/main.cpp -o DoceHamburgueria
```

Nesse caso, coloque `src/supabase.ps1` e o `.env` ao lado do executável gerado. Essa compilação com g++ não solicita vinculação estática e também depende de `pwsh` para acessar o Supabase.

## Estrutura e diagramas

- `src/main.cpp`: interface de terminal, validações, cálculos e relatórios.
- `src/supabase.hpp`: classe C++ que gerencia o processo auxiliar e o protocolo de comunicação por pipes.
- `src/supabase.ps1`: configuração, autenticação, renovação da sessão e chamadas HTTPS às RPCs do Supabase.
- `iniciar.ps1`: inicialização no Windows com solicitação de senha oculta.
- `compilar.ps1`: compilação dos executáveis e cópia dos arquivos auxiliares.
- [Diagramas de classes e de sequência](docs/diagramas.md): documentação com arquivos PlantUML (.puml) para visualização gráfica.

# Diagramas UML — Doc'ê Hamburgueria

Os diagramas estão em arquivos PlantUML (`.puml`), com cores por responsabilidade, agrupamentos e legendas. Foram modelados a partir de `src/main.cpp`, `src/supabase.hpp` e `src/supabase.ps1`.

## Abrir a visualização gráfica

No VS Code com a extensão PlantUML instalada, abra um dos arquivos abaixo e use **Alt+D** ou o comando **PlantUML: Preview Current Diagram** na paleta de comandos.

- [Diagrama de classes — classes.puml](classes.puml)
- [Diagrama de sequência — sequencia.puml](sequencia.puml)

O projeto configura a extensão para usar `https://www.plantuml.com/plantuml` em `.vscode/settings.json`. Não é necessário Java local; a prévia exige internet e envia o conteúdo do diagrama ao servidor público para renderização.

Se o projeto estiver aberto em um workspace com várias pastas, a versão 2.18.1 da extensão lê essas opções no nível do workspace. Nesse caso, adicione também em **Preferences: Open Workspace Settings (JSON)**, dentro de `settings`:

```json
"plantuml.render": "PlantUMLServer",
"plantuml.server": "https://www.plantuml.com/plantuml"
```

As imagens já geradas também podem ser abertas diretamente: [classes.svg](classes.svg) e [sequencia.svg](sequencia.svg). Após alterar os `.puml`, exporte novamente para atualizar essas imagens.

Para gerar uma imagem, use **PlantUML: Export Current Diagram** e escolha SVG ou PNG. A prévia e a exportação dependem do renderizador configurado na extensão. Os arquivos também podem ser abertos em outros editores compatíveis com PlantUML.

## Diagrama de classes

Mostra a classe `Supabase`, a estrutura `Entry` e as dependências entre o terminal C++, o auxiliar PowerShell e o servidor remoto.

Os elementos identificados como módulos agrupam funções livres existentes; não são classes implementadas. `Money` é um alias de `std::int64_t`, com valores em centavos. Não há herança no projeto, e o saldo é calculado a partir das movimentações.

O servidor é representado pelos contratos chamados pelo cliente. As definições SQL das RPCs e das tabelas não estão neste repositório, portanto seus relacionamentos internos não são presumidos.

## Diagrama de sequência

Mostra o início da aplicação e o registro de uma entrada ou saída, com autenticação, consulta do histórico, validações, confirmação de saldo negativo, gravação remota e tratamento de erro da RPC. As etapas são numeradas e separadas por fase.

A autenticação ocorre na primeira RPC. Antes de cada RPC, o auxiliar verifica a sessão e renova o token quando necessário. Uma falha de comunicação após o envio pode deixar o resultado da gravação incerto; o cliente não repete o cadastro automaticamente.

Outros fluxos implementados: consulta do extrato usa `load()`; exportação usa `load()` e gera `extrato.csv` e `extrato.xml`; exclusão exige `s` e `APAGAR`, chama `clear()` → `DELETE` → `finance_clear` e depois remove os relatórios locais. O destrutor de `Supabase` envia `QUIT` e fecha a comunicação com o auxiliar.

As movimentações são armazenadas no Supabase. CSV e XML são apenas relatórios locais gerados por solicitação.

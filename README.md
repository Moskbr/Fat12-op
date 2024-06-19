# Sistema de Arquivos FAT12
Trabalho de Sistemas Operacionais: Implementação básica de um sistema de arquivos FAT12.

O sistema é capaz de listas os arquivos nos diretórios e subdiretórios de um disco FAT12, além de copiar os arquivos no disco para o sistema, com o comando "grab".

Para a implementação dos subdiretórios foi utilizado uma lista ligada contendo um vetor das entradas dentro dela, e informações como "path" e o índice do diretório pai de vetor das entradas root.
Por isso, nele é possível ter dois níveis de diretórios (root/subdir) com no máximo 14 entradas em cada um, e sem limites para quantidade de subdiretórios, estruturados pela lista ligada.

Referências:
- FAT12 Overview: https://www.eit.lth.se/fileadmin/eit/courses/eitn50/Projekt1/FAT12Description.pdf
- A tutorial on the FAT file system: https://www.tavi.co.uk/phobos/fat.html

Bug conhecido: comando "cp" (para copiar um aquivo do sistema no disco) não funciona

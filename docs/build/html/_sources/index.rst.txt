.. Flow Data Storage documentation master file, created by
   sphinx-quickstart on Mon May 29 10:19:20 2017.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to Flow Data Storage's documentation!
=============================================

.. toctree::
   :maxdepth: 2
   :caption: Contents:



File Format Structure
=====================

Struktura souboru pro ukládání flow dat [DRAFT]
Dokument popisuje návrh nové datové struktury pro ukládání flow dat získaných pomocí protokolu IPFIX a NetFlow.
Požadavky
Před samotným návrhem je nutné si v krátkosti uvědomit některé požadavky, které musí nová struktura splňovat oproti současnému řešeni (tj. oproti ukládání do nfdump formátu). Uveďme si některé:
Podpora položek dynamické délky
Snadné přidávání nových datových položek toků (např. HTTP hlavičky, apod.)
Jednoduchá rozšiřitelnost o nové funkcionality
Struktura
Každý soubor se skládá z hlavičky, která je následována bloky různých významových typů
(šablony, flow data, statistiky, informace o exportéru atd.). Jednotlivé bloky jsou popsány v částech níže.


	+--------+---------+---------+-----+---------+-----------+
	|  File  |  Block  |  Block  | ... |  Block  | Extension |
	| Header |    1    |    2    |     |    N    |   Table   |
	+--------+---------+---------+-----+---------+-----------+


Obecná struktura souboru

Hlavička souboru
Jako první a vždy přítomná struktura souboru je tzv. hlavička souboru, která obsahuje:
Magic	- konstanta pro identifikaci datového souboru a správné endianity
Version	- verze formátu souboru (pro případné budoucí nekompatibilní změny)
Flags	- globální příznaky (např. zvolený algoritmus komprimace bloků, apod.)
Number of blocks - celkový počet bloků v souboru
Block table offset - offset na tabulku obsahující pozice (offsety) významných bloků v souboru. Pokud není tabulka v souboru přítomna, obsahuje hodnotu 0.

::

	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|             Magic             |            Version            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                             Flags                             |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                        Number of blocks                       |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                      Extension table offset                   |
	|                             (64b)                             |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Hlavička souboru
Společná hlavička bloků
Každý blok musí obsahovat společnou a pevně definovanou hlavičku, která slouží k určení typu bloku a jeho délky, což umožní čtecímu nástroji přeskočit bloky, kterým nebude rozumět. Může se jednat i o situaci, kdy starší verze nástroje čte soubory generované novějším nástrojem, který přidává rozšiřující bloky, které nemění způsob ukládání flow dat (např. přidává blok s indexy) a čtecí nástroj je může bezpečně přeskočit.

::

	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|             Type              |             Flags             |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                             Length                            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                                                               |
	|                ~ ~ ~ Content of the block ~ ~ ~               |
	|                                                               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Hlavička bloku
 
Kde
Type 	- identifikace typu bloku (šablony, flow data, informace o exportéru, apod.)
Flags	- příznaky (např. zda daný blok je komprimovaný)
Length	- délka bloku včetně hlavičky
 

Bloky
Následující části popisují jednotlivé bloky z pohledu významu a jejich struktury.
1. Blok identifikace exportéru (Exporter information block)
Vzhledem k tomu, že jeden soubor může obsahovat flow data zachycená od více exportérů je vhodné být schopný identifikovat zdroj dat a jeho vlastnosti (např. aktivní vzorkování). Jelikož jsou zdroje dat odkazovány v hlavičce bloků flow dat (viz dále), je nutné, aby se blok s informacemi o exportéru vyskytoval v souboru před prvním blokem, který na něj odkazuje.
 
V případě, že nastala situace, kdy exportér není znám nebo nemá význam ho uvádět, bude existovat implicitní záznam s ID 0, který agreguje informace od těchto neznámých zdrojů.
 
TODO: V rámci struktury je třeba dodefinovat nutné položky sledované v rámci vzorkování.

::

	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                           Exporter ID                         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                               ODID                            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                                                               |
	|                       IPv4 / IPv6 address                     |
	|                                                               |
	|                                                               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                         Sampling  (TODO)                      |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                                                               |
	~                         Description (64B)                     ~
	 
	~                                                               ~
	|                                                               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Struktura bloku s informací o exportéru (bez společné hlavičky)
 
Samotná struktura by měla obsahovat následující položky
Exporter ID 	- unikátní identifikační číslo exportéru v rámci celého souboru
ODID 	- Observation Domain ID (viz definice dle IPFIX)
IP address 	- IPv4/IPv6 adresa exportéru, kde IPv4 je zakódována jako “IPv4-Mapped IPv6 Address” (RFC 4291, sekce 2.5.5.2.)
Sampling 	- metoda a frekvence vzorkování
Description 	- popis exportéru (např. název linky,...)
 
2. Blok šablon (Template block)
Pro zajištění podpory flexibilního formátu s podporou dynamických položek je nutné popsat strukturu flow dat za pomoci šablon. Funkcí šablony je určit, které datové položky se v záznamu nachází (např. zdrojová + cílová adresa, počet bytů, URL adresa,...), na jaké pozici a jejich velikost. Jelikož mohou od exportéru přicházet data obohacená o různé informace, je nutné podporovat současné použití více šablon - některé toky např. HTTP provoz může být obohacen o URL adresu, ale v jiné situaci (obecný provoz) se tyto položky nevyskytují. Šablony nejsou navázány na zdroj dat - jinými slovy, jedna šablona může být používána více exportéry.
 
V rámci jednoho bloku šablon je možné definovat 1 až N šablon, kde každá šablona je identifikována jednoznačným číslem a je platná od své definice dále s tím, že není možné ji nijak předefinovat/zrušit. Z toho plyne, že definice šablony se musí vyskytovat v souboru dříve než blok flow dat, který je jí popsán.

::

	+---------------------------------------------------------------+
	|       Common Block header (type == LNF_FILE_BLOCK_TMPLT)      |
	+---------------------------------------------------------------+
	|                         Template Record 1                     |
	+---------------------------------------------------------------+
	|                         Template Record 2                     |
	+---------------------------------------------------------------+
	|                                ...                            |
	+---------------------------------------------------------------+
	|                         Template Record N                     |
	+---------------------------------------------------------------+

Struktura bloku se šablonami

Každá šablona v rámci bloku obsahuje hlavičku s číslem šablony a počtem položek, které definuje. Pro budoucí využití je zde rezervována 2 bytová položka, jejíž hodnota musí být v tuto chvíli nulová. Za touto hlavičkou je umístěn seznam identifikátorů datových položek, kde pořadí je směrodatné a určuje posloupnost položek ve flow záznamu. Z důvodu optimalizace přístupu k datovým položkám (viz. blok flow dat) musí být prve uvedeny všechny položky fixní délky a až následně položky dynamické délky. Prokládání těchto položek v rámci posloupnosti není dovoleno.

::

	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                           Template ID                         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|           Field count         |          ~ reserved ~         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                                                               |
	|        ~ ~ ~ One or more Template field specifiers ~ ~ ~      |
	|                                                               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Struktura šablony
 
Každá položka v rámci šablony je určena na základě Enterprise number (identifikace vlastníka rozsahu) a ID (identifikace položky v rámci rozsahu). Vychází se tudíž z definice převzaté z protokolu IPFIX, kde základní položky (zdrojová/cílová adresa, počet bytů/paketů/flow, apod.) jsou definovány a udržovány v seznamu IANA (Enterprise number 0) a specifické položky jednotlivých vlastníků (např. URL adresa, informace o SIP, atd.) jsou pak spravovány samotnými organizacemi.

::

	0                   1                   2                   3  _
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1_
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Enterprise number                       |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|           Field ID            |              Length           |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Struktura identifikace datové položky v šabloně
 
Pro každou položku je uvedena také délka určující fixní délku ve flow záznamu. V případě, že délka položky není předem známá tj. její velikost je dynamická, má délka hodnotu UINT16_MAX (65535).
 
3. Blok flow dat (Flow data block)
Datový blok se skládá ze speciální hlavičky, kterou následuje 1 až N flow záznamů. Speciální hlavička obsahuje informace o šabloně, která byla použita při zápisu všech záznamů v rámci tohoto bloku, a identifikační číslo exportéru, který záznamy poslal. V případě aktivní komprese bloků jsou komprimována až data uvedená za hlavičkou tj. samotná speciální hlavička komprimována není. Důvodem je možnost optimalizace vyhledávání přítomnosti datových položek v blocích, kdy při filtraci se na základě analýzy šablon určí, zda se položka v šabloně nachází, a má tudíž blok popsaný touto šablonou smysl dekomprimovat.

::

	+---------------------------------------------------------------+
	|      Common Block header (type == LNF_FILE_BLOCK_FLOW)        |
	+---------------------------------------------------------------+
	|                        Data block header                      |
	+---------------------------------------------------------------+
	|                          Flow Record 1                        |
	+---------------------------------------------------------------+
	|                          Flow Record 2                        |
	+---------------------------------------------------------------+
	|                              ...                              |
	+---------------------------------------------------------------+
	|                          Flow Record N                        |
	+---------------------------------------------------------------+

Struktura bloku flow dat

::

	0                   1                   2                   3  _
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1_
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                          Template ID                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                          Exporter ID                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Speciální hlavička bloku flow dat
 
Struktura dat uložených v datech vychází z příslušné šablony a může vypadat např. následovně (v závislosti na použité šabloně)::

	0                   1                   2                   3   _
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 _
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|          Record Length        |         Field Value 1         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                         Field Value 2                         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|          Field Value 3        |         Field Value 4         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|          Field Value 5        |         Field Value 6         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                         Field Value 7                         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|          OFFSET Field 8       |         LENGTH Field 8        |  Static
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  part
	|          OFFSET Field 9       |         LENGTH Field 9        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ - - - -
	|                         Field Value 8                         |
	+                                               +-+-+-+-+-+-+-+-+  Dynamic
	|                                               |               |  part
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+               +
	|                         Field Value 9                         |
	+                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                               |                               _
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               _
 
Záznam na začátku obsahuje 2B hodnotu určující délku samotného záznamu a dále je strukturovaný v závislosti na šabloně. Samotný záznam je možné rozdělit na 3 části:
Hodnoty položek pevné délky
Meta informace o položkách dynamické délky
Hodnoty položek dynamické délky
 
Kde druhá část obsahuje pro každou dynamickou položku její offset od začátku záznamu a délku v bytech. Meta informace a dynamických položky jsou také seřazeny dle pořadí, které bylo uvedeno v šabloně. V rámci záznamu by se neměla vyskytovat prázdná nevyužitá místa. 
 
První dvě části záznamu je možné pojmenovat jako sekci pevné délky, neboť jejich celková velikost je vždy stejná a všechna data jsou přítomna na pevných offsetech daných šablonou, což umožňuje rychlý přístup k datům při čtení. Poslední část je dynamická, jelikož její celková velikost a počáteční pozice jednotlivých hodnot se mohou vyskytovat na různých místech u záznamů dle stejné šablony.
 
(Poznámka: struktura a popis ukládaného záznamu vychází z formátu a dokumentace 
UniRec používaném v rámci Nemea Frameworku).
 
4. Tabulka offsetů bloků (Block offset table)
Pro možnost snadno zjistit pozici významných bloků bez nutnosti procházet a zpracovat celý soubor při jeho čtení, bude v rámci každého souboru existovat nejvýše jedna tabulka offsetů bloků. Mezi sledované bloky patří všechny bloky, kromě bloků šablon a flow dat tj. např. bloky identifikace exportéru, bloky statistik, bloky indexů (není součástí současného návrhu), apod. Mezi tyto bloky tudíž budou patřit i bloky specifikované v budoucí revizi.
 
Jakmile v rámci souboru existuje alespoň jeden ze sledovaných bloků, musí tabulka offsetů existovat a v tabulce musí být uveden právě jednou offset všech sledovaných bloků, které jsou v souboru obsaženy. Čtecí proces se tak může spolehnout, že žádný takový blok nebude “skryt”, a nemusí tudíž sekvenčně procházet všechny bloky souboru.
 
Jelikož množství bloků může v průběhu vzniku souboru přibývat a nelze tedy předem určit jejich množství a pozici, musí být tato tabulka vložena až na samotný konec souboru. Pro rychlý přístup k této tabulce offsetů se pak v hlavičce souboru nachází odkaz (offset) na její začátek (vyplněn při uzavírání souboru).

::

	+---------------------------------------------------------------+
	|    Common Block header (type == LNF_FILE_BLOCK_OFFSET_TBL)    |
	+---------------------------------------------------------------+
	|                       Block Offset Record 1                   |
	+---------------------------------------------------------------+
	|                       Block Offset Record 2                   |
	+---------------------------------------------------------------+
	|                               ...                             |
	+---------------------------------------------------------------+
	|                       Block Offset Record N                   |
	+---------------------------------------------------------------+
Struktura tabulky offsetů bloků
 
Každý záznam tabulky popisuje typ bloku a jeho offset. 

::

	0                   1                   2                   3  _
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1_
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|            Type               |                               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               +
	|                          Block Offset                         |
	+                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                               |                               _
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               _
Záznam tabulky offsetů bloků
 
5. Bloky statistik 
Pro rychlé statistické informace o tocích obsažených v rámci daného souboru, budou na konci souboru (před tabulkou offsetů bloků) umístěny bloky statistik jednotlivých exportérů. 
 
Blok statistik obsahuje identifikační číslo exportéru a k němu náležící statistiky:
celkový počet toků/bytů/paketů
počet TCP/UDP/ICMP/ostatních toků
počet TCP/UDP/ICMP/ostatních bytů
počet TCP/UDP/ICMP/ostatních paketů
 
TODO:
Přesná struktura bloku 
Objevily se tendence zapracovat generickou strukturu statistik umožnující detailněji specifikovat statistiky dle aktuálních požadavků (např. pro daný port, atd.), kde by se také pravděpodobně zužitkovaly šablony.
Možnost mít více statistik od jednoho zdroje v rámci souboru - např. v rámci 5 minutového souboru by byly přítomny statistiky po 1 minutě, což by v důsledku znamenalo, že už by bloky statistik nemusely být na konci souboru (tj. mohly by být rozprostřené). Otázka je zda to má význam, neboť některé toky trvají kratší dobu (jednotky sekund) a jiné delší dobu (jednotky minut), přičemž o delších tocích se dozvíme později až je např. statistiky zaznamenána přičemž náleží do příslušného minutového intervalu. 


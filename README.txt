Filip Emil Florentin - 323CB
<filipemil02@yahoo.com>
16.05.2023

Programul incepe prin setarea output-ului ca unbuffered pentru a scrie imediat pe ecran.Iau portul transmis ca parametru
Deschid porturile de tcp si udp si le dau bind la port iar la cel de tcp il pun pe listen pentru a asculta mesajele viitoare.
Initializez 2 tabele de file descriptors, una unde stochez toate file descriptors si una temporara de care am avut nevoie la apelul
functiei select pentru ca in urma apelului, se va modifica parametrul(temporar).Am dezactivat algoritmul lui Nagle pentru ca serverul
sa fie rapid la trimiterea de pachete.
Cat timp serverul nu este inchis, cu ajutorul functiei select vad pe ce port s-au facut modificari.
Daca portul este de tcp, inseama ca am un request de conectare si verific daca nu cumva este conectat deja.In caz afirmativ, afisez un
mesaj corespunzator si refuz request-ul.In caz negativ, il conectez la server si verifica daca cumva a mai fost activ si a dat subscribe
la vreun canal care a facut postari in timpul in care a fost plecat pentru a i le afisa.
Daca portul este de udp, parsez mesajul si il trimit tuturor clientilor conectati care au dat subscribe la acel canal si il si salvez in
caz ca am avut niste clienti care au dat subscribe si sunt offline , in cazul in care au optiunea Sf activata.
Daca am activitate de la tastatura, verific daca este mesajul de inchidere al server-ului (exit) pentru a-l inchide.
In ultimul caz, iau mesajul primit si inchid conexiunea daca rezultatul intors de functia receiveMessage este 0 pentru ca inseamna ca acel
client s-a deconectat.Iterez prin clienti pentru a vedea pentru care dintre acestia a fost transmis mesajul si salvez il salvez in index.
Daca mesajul a fost de deconectare, salvez ca a fost online si afisez mesajul corespunzator iesirii clientului.
Daca mesajul a fost de subscribe/unsubscribe fac operatia aferenta mesajului primit.
In subscribe, iau portul transmis ca parametru si deschid un socket.File descriptors au aceeasi insemnatate ca in main.Cat timp este serverul
deschis verific daca am comenzi de la alti socketi sau de la tastatura pentru subscribe unsubscribe si exit.Daca am primit un mesaj de la server, printez mesajul in pattern-ul din enunt.
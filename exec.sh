# 1. COMPILAZIONE
# Il comando 'make' necessita del makefile

  make clean

  clear

  make

  read -p "Compilazione eseguita. Premi invio per eseguire..."

# 2. ESECUZIONE
# I file eseguibili di discovery server e peer devono
# chiamarsi 'ds' e 'peer', e devono essere nella current folder

# 2.1 esecuzioe del DS sulla porta 4242
  gnome-terminal -x sh -c "./ds 4242; exec bash"

# 2.2 esecuzione di 5 peer sulle porte {5001,...,5005}
  for port in {5001..5004}
  do
     gnome-terminal -x sh -c "./peer $port; exec bash"
  done

echo
echo === INSTALAR PIP3 ===
apt-get update && apt-get install -y python3-pip
echo

echo === INSTALAR DEPENDENCIAS WEB SERVICE ===
pip3 install spyne lxml zeep
clear

echo === ARRANCAR WEB SERVICE EN BACKGROUND ===
python3 web_service.py > /dev/null 2>&1 &
sleep 5
echo

echo === FIN DE LA PREPARACIÓN ===
echo

python3 client.py -s $1 -p $2
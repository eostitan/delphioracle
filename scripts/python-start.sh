## Docker initial entry script

conf_setup() {

cd /app
echo "Adding private key to .env"
sed -i "s/<replace with private key>/$PRIVATE_KEY/" oracleconf.py 
echo "Adding Block Producer to oracleconf.py"
sed -i "s/bpname/$BPNAME/" oracleconf.py 
echo "Adding permission"
sed -i "s/permission/$PERM/" oracleconf.py 
echo "Setting API"
sed -i "s/<replace with api endpoint>/$API/" oracleconf.py
echo "Setting API PORT"
sed -i "s/<replace with api port>/$APIPORT/" oracleconf.py
echo "Setting API KEY"
sed -i "s/<replace with APIkey>/$APIKEY/" oracleconf.py


# Setting chain ID
waxid="1064487b3cd1a897ce03ae5b6a865651747e2e152090f99c1d19d44e01aea5a4"
eosid="aca376f206b8fc25a6ed44dbdc66547c36c6c33e3a119ffbeaef943642f0e906"

echo "Setting the chain ID"
if [[ $CHAIN = "wax" ]]
then
  sed -i "s/<replace with chain id>/$waxid/" oracleconf.py
  sed -i "s/<replace with chain name>/wax/" oracleconf.py
fi
if [[ $CHAIN == 'eos' ]]
then
    sed -i "s/<replace with chain id>/$eosid/" oracleconf.py
    sed -i "s/<replace with chain name>/eos/" oracleconf.py
fi

}

run_oracle(){
while [ 1 ]
do
	cd /app
	python3 updater.py
	sleep 60
done
}

# Running functions
conf_setup
run_oracle



## Docker initial entry script

env_setup() {

cd /app
echo "Adding private key to .env"
sed -i "s/<replace with private key>/$PRIVATE_KEY/" .env 
echo "Adding Block Producer to .env"
sed -i "s/bpname/$BPNAME/" .env 
echo "Adding permission"
sed -i "s/permission/$PERM/" .env 
echo "Setting API"
sed -i "s/<replace with api endpoint>/$API/" .env
echo "Setting API PORT"
sed -i "s/<replace with api port>/$APIPORT/" .env

# Setting chain ID
waxid="1064487b3cd1a897ce03ae5b6a865651747e2e152090f99c1d19d44e01aea5a4"
eosid="aca376f206b8fc25a6ed44dbdc66547c36c6c33e3a119ffbeaef943642f0e906"

echo "Setting the chain ID"
if [[ $CHAIN = "wax" ]]
then
  sed -i "s/<replace with chain id>/$waxid/" .env
  sed -i "s/<replace with chain name>/wax/" .env
fi
if [[ $CHAIN == 'eos' ]]
then
    sed -i "s/<replace with chain id>/$eosid/" .env
    sed -i "s/<replace with chain name>/eos/" .env
fi

}

run_oracle(){
while [ 1 ]
do
	cd /app
	node updater.js
	sleep 60
done
}

# Running functions
env_setup
run_oracle



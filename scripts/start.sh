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
waxid_mainnet="1064487b3cd1a897ce03ae5b6a865651747e2e152090f99c1d19d44e01aea5a4"
waxid_testnet="f16b1833c747c43682f4386fca9cbb327929334a762755ebec17f6f23c9b8a12"
eosid_mainnet="aca376f206b8fc25a6ed44dbdc66547c36c6c33e3a119ffbeaef943642f0e906"
eosid_testnet="2a02a0053e5a8cf73a56ba0fda11e4d92e0238a4a2aa74fccf46d5a910746840"

echo "Setting the chain ID"
if [[ $CHAIN = "wax" ]]
then
  if [[ $ENV = "mainnet" ]]
  then
      sed -i "s/<replace with chain id>/$waxid_mainnet/" .env
      sed -i "s/<replace with chain name>/wax/" .env
  elif [[ $ENV = "testnet" ]]
  then
      sed -i "s/<replace with chain id>/$waxid_testnet/" .env
      sed -i "s/<replace with chain name>/wax/" .env
  fi
fi

if [[ $CHAIN = "eos" ]]
then
  if [[ $ENV = "mainnet" ]]
  then
      sed -i "s/<replace with chain id>/$eosid_mainnet/" .env
      sed -i "s/<replace with chain name>/wax/" .env
  elif [[ $ENV = "testnet" ]]
  then
      sed -i "s/<replace with chain id>/$eosid_testnet/" .env
      sed -i "s/<replace with chain name>/wax/" .env
  fi
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

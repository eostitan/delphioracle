import json
import requests
import eospyo
import oracleconf as cfg
from pydantic import HttpUrl

EOS_PROTOCOL = cfg.main["EOS_PROTOCOL"]
EOS_HOST = cfg.main["EOS_HOST"]
EOS_PORT = cfg.main["EOS_PORT"]
EOS_KEY = cfg.main["EOS_KEY"]
EOS_CHAIN = cfg.main["EOS_CHAIN"]
ORACLE = cfg.main["ORACLE"]
CONTRACT = cfg.main["CONTRACT"]
ORACLE_PERMISSION = cfg.main["ORACLE_PERMISSION"]
FREQ = cfg.main["FREQ"]
CHAIN = cfg.main["CHAIN"]
APIKEY = cfg.main["APIKEY"]


class MyNetwork(eospyo.Net):
    host: HttpUrl = EOS_HOST 


#data = '[ 

auth = eospyo.Authorization(
    actor=ORACLE,
    permission=ORACLE_PERMISSION,
)


def getPrice():
    response = requests.get("https://min-api.cryptocompare.com/data/pricemulti?fsyms=USD,WAXP&tsyms=USDT,USDC,BTC,USD,ETH,EOS&api_key="+APIKEY)
    json_response = response.json()
    BTC = round(json_response['WAXP']['BTC']*100000000)
    USD = round(json_response['WAXP']['USD']*10000)
    EOS = round(json_response['WAXP']['EOS']*1000000)
    ETH = round(json_response['WAXP']['ETH']*100000000)
    USDUSDT = round(json_response['USD']['USDT']* 10000)
    USDUSDC = round(json_response['USD']['USDC']* 10000)
    data = [
        {'value': BTC, 'pair': 'waxpbtc' }, 
        {'value': USD, 'pair': 'waxpusd' },
        {'value': EOS, 'pair': 'waxpeos' },
        {'value': ETH, 'pair': 'waxpeth' }, 
        {'value': USDUSDT, 'pair': 'usdtusd' },
        {'value': USDUSDC, 'pair': 'usdcusd' }
    ]
    print(data)
    return data


pricedata = getPrice()

action = eospyo.Action(
    account=CONTRACT,  # this is the contract account
    name="write",  # this is the action name
    data={
            'owner': ORACLE,
            'quotes': pricedata
            },
    authorization=[auth],
)

raw_transaction = eospyo.Transaction(actions=[action])


print("Link transaction to the network")
#net = eospyo.WaxMainnet()
net = MyNetwork()
# notice that eospyo returns a new object instead of change in place
linked_transaction = raw_transaction.link(net=net)


print("Sign transaction")
key = EOS_KEY
signed_transaction = linked_transaction.sign(key=key)


print("Send")
resp = signed_transaction.send()

print("Printing the response")
resp_fmt = json.dumps(resp, indent=4)
print(f"Response:\n{resp_fmt}")

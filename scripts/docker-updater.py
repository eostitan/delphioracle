import json
import requests
import eospyo
import oracleconf as cfg


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


#data = '[ 

auth = eospyo.Authorization(
    actor=ORACLE,
    permission=ORACLE_PERMISSION,
)


def getPrice():
    response = requests.get("https://min-api.cryptocompare.com/data/price?fsym=WAXP&tsyms=BTC,USD&api_key="+APIKEY)
    json_response = response.json()
    BTC = round(json_response['BTC']*100000000)
    USD = round(json_response['USD']*1000)
    data = [{'value': BTC, 'pair': 'waxpbtc' }, {'value': USD, 'pair': 'waxpusd' }]
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
net = eospyo.WaxMainnet()
# notice that eospyo returns a new object instead of change in place
linked_transaction = raw_transaction.link(net=net)


print("Sign transaction")
key = EOS_KEY
signed_transaction = linked_transaction.sign(key=key)


print("Send")
resp = signed_transaction.send()

#print("Printing the response")
#resp_fmt = json.dumps(resp, indent=4)
#print(f"Response:\n{resp_fmt}")

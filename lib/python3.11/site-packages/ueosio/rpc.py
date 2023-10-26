import os
import sys
import requests as r
import json
import traceback
import logging
from collections import OrderedDict

RPC_NODE = os.environ.get('RPC_NODE', 'http://127.0.0.1:8888')
TIMEOUT = os.environ.get('TIMEOUT',10)

class ApiNameWrapper:
  def __init__(self, host, api_version,timeout, api_name):
    self.host = host
    self.api_version = api_version
    self.timeout = timeout
    self.api_name = api_name

  def __getattr__(self, attr):
    
    def call_name(*args, **kwargs):
      headers = {'content-type':'application/json'}
      url = "{0}/{1}/{2}/{3}".format(self.host, self.api_version, self.api_name, attr)
      
      if len(args) > 0 and len(kwargs) > 0:
        raise Exception("bad args for {0}".format(attr))

      if len(kwargs) > 0:
        payload=json.dumps(kwargs)
      elif len(args) == 1:
        payload=json.dumps(args[0])
      else:
        payload=json.dumps(list(args))
      res = r.post(url, data=payload, headers=headers, timeout=self.timeout)

      return json.loads(res.text, object_pairs_hook=OrderedDict)
    return call_name

class ApiVerWrapper:
  def __init__(self, host, timeout, api_version):
    self.host = host
    self.timeout= timeout
    self.api_version = api_version

  def __getattr__(self, attr):
    return ApiNameWrapper(self.host, self.api_version, self.timeout, attr)

class Api(object):

  def __init__(self, host=RPC_NODE, timeout=TIMEOUT):
    self.timeout = timeout
    self.host = host

  def set_host(self, host):
    self.host = host

  def __getattr__(self, attr):
    return ApiVerWrapper(self.host, self.timeout, attr)

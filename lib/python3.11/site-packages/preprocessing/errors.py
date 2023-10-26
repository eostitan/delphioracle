'''
Error handling module with classes:

- Error
    - base errpr handle
- FunctionError
    - error handle for incorrect functions passed as arguments
- InputError
    - error handle for incorrect function arguments
'''

class Error(Exception):
    '''base error handle'''
    pass

class FunctionError(Error):
    '''error handle for incorrect functions passed as arguments'''
    pass

class InputError(Error):
    '''error handle for incorrect function arguments'''
    pass

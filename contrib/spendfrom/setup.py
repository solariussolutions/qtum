from distutils.core import setup
setup(name='qtumspendfrom',
      version='1.0',
      description='Command-line utility for quantum "coin control"',
      author='Gavin Andresen',
      author_email='gavin@quantumfoundation.org',
      requires=['jsonrpc'],
      scripts=['spendfrom.py'],
      )

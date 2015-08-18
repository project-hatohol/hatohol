
from distutils.core import setup

setup(name='hatohol',
      description='Hatohol arm plugin 2.0',
      author='Procect Hatohol',
      author_email='hatohol-users@list.sourceforge.net',
      url='https://github.com/project-hatohol/hatohol',
      py_modules=['hatohol.rabbitmqconnector', 'hatohol.transporters.rabbitmqhapiconnector']
     )

from distutils.core import setup, Extension
import os
setup (name = 'picam',
       version = '0.1',
       author='Sean Ashton',
       author_email='sean.ashton@schimera.com',
       description = 'Raspberry Pi Camera Module Python Library',
       packages=['picam'],      
       package_data={'picam':['_picam.so']},
       )
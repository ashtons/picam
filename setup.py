from distutils.core import setup, Extension
import os

module1 = Extension('picam._picam',
                    define_macros = [('MAJOR_VERSION', '1'),
                                     ('MINOR_VERSION', '0')],
                    include_dirs = ['/usr/local/include','/opt/vc/include','/opt/vc/include/interface/vcos/pthreads','/opt/vc/include/interface/vmcs_host/linux/'],
                    libraries = ['mmal','vcos','bcm_host'],
                    library_dirs = ['/usr/local/lib','/opt/vc/lib'],
                    sources = ['./src/picammodule.c','./src/picam.c','./src/RaspiCamControl.c'])

setup (name = 'picam',
       version = '1.0',
       author='Sean Ashton',
       author_email='sean.ashton@schimera.com',
       description = 'Raspberry Pi Camera Module Python Library',
       license='MIT',
       download_url='https://github.com/ashtons/picam',
       packages=['picam'],  
       package_data={'picam':['_picam.so']},
       ext_modules = [module1]
       )

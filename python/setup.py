from setuptools import setup, find_packages

# TODO expand this to be useful!
setup(name='branchedflowsim',
      version='0.1.1',
      packages=find_packages(),
      install_requires=['numpy', 'scipy'],
      tests_require=["pytest"],
      scripts=['density_image', 'viewvolume']
)  

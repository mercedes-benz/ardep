.. _documentation:
   

Documentation
#############

The main documentation is build with `sphinx <https://www.sphinx-doc.org/en/master/>`_. Code docs are generated using `doxygen <https://www.doxygen.nl/>`_.

Generally, we try to keep the documentation as close to the code as possible, so it's easier to keep it up to date:

- Board specific documentation in */boards/arm/ardep*
- samples specific documentation sample-specific directory under  */samples*
- scripts and ``west`` subcommands in */scripts*
- generic documentaion in */doc*


Building the documentation
==========================

Install necessary dependencies:

.. code-block:: bash

    pip install -r doc/requirements.txt
    apt install doxygen rsync

To build the html documentation simply run:

.. code-block:: bash

    make html


This copies all \*.rst and associated media files from the project root into */doc/_board* directory, so sphinx can find and correctly include these files


Live-preview (sort of...)
==========================

To archive something like a hot-reload / live-preview, you can use a command like this from inside the *doc* directory:

.. code-block:: bash

    sphinx-autobuild \
        --pre-build "mkdir -p _build/html"\
        --pre-build "doxygen"\
        . _build/html

The pre-build commands are taken from the Makefile.

This automatically copies all files and rebuilds the documentation, if sphinx notices a change.
But since sphinx only works on a copy of the original files, changes outside of the *doc* directory will not be noticed.

To force a rebuild, *save* or `touch` a file inside the *doc* directory


Including files above the *doc* directory
=========================================

In sphinx you can't include a file above the sphinx root directory (here */doc*) in the toctree.
To bypass this, we copy all \*.rst and related media files into the */doc/_board* directory before the build using rsync.


This means for example, if you want to reference the uds sample in */samples/uds/index.rst* you would reference it in the toctree as *_board/samples/uds/index.rst*

When you add a media file, make sure it is copied by rsync. If necessary, update the command in the Makefile.
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

.. tabs::

    .. tab:: Linux

        .. code-block:: bash

             pip install -r doc/requirements.txt
             apt install doxygen rsync

    .. tab:: Windows

        - Install the python requirements with:

            .. code-block:: powershell

                 pip install -r doc/requirements.txt

        - Install doxygen from the following link: https://www.doxygen.nl/download.html
        - The ``rsync`` command will be used through the `Windows Subsystem for Linux (WSL) <https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux>`_.
        
To build the API Reference run:

.. code-block:: bash

    doxygen

To build the html documentation run:

.. code-block:: bash

    make html


This copies all \*.rst and associated media files from the project root into */doc/_board* directory, so sphinx can find and correctly include these files.

If you don't want to copy any files, you can run ``make html SYNC=off`` instead:


Live-preview (sort of...)
==========================

To archive something like a hot-reload / live-preview, you can use a command like this from inside the *doc* directory:

.. tabs::

    .. tab:: Linux
        .. code-block:: bash

            sphinx-autobuild \
                --pre-build 'rsync -avm --exclude="/doc" --include="*/" --include="*.rst" --include="*.png" --exclude="*" .. _board' \
                --pre-build "mkdir -p _build/html"\
                --pre-build "doxygen" \
                . _build/html

    .. tab:: Windows

        .. code-block:: powershell

            sphinx-autobuild `
                --pre-build "wsl rsync -avm --exclude=/doc --include=*/ --include=*.rst --include=*.png --exclude=* .. _board" `
                --pre-build "python -c \"import pathlib; pathlib.Path('_build/html').mkdir(parents=True, exist_ok=True)\"" `
                --pre-build "doxygen" `
                . _build/html

The pre-build commands are taken from the Makefile.

This automatically copies all files and rebuilds the documentation, if sphinx notices a change.
But since sphinx only works on a copy of the original files, changes outside of the *doc* directory will not be noticed.

To force a rebuild, *save* or `touch` a file inside the *doc* directory.

If you want to omit the file sync and api-reference build, simply call the command without the ``--pre-build`` options.


Including files above the *doc* directory
=========================================

In sphinx you can't include a file above the sphinx root directory (here */doc*) in the toctree.
To bypass this, we copy all \*.rst and related media files into the */doc* directory before the build using rsync.
The directory structure should be preserved, so referencing a file in the toctree should follow the same path as in the repository (e.g. *samples/uds/index.rst* to reference the uds sample).

When you add a media file, make sure it is copied by rsync. If necessary, update the command in the Makefile.
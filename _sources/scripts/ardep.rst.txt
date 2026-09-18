.. _ardep_subcommand:

ARDEP Command
################

The Zephyr RTOS has its own meta-tool called `west <https://docs.zephyrproject.org/latest/develop/west/index.html>`_.

ARDEP comes with the handy subcommand ``ardep`` for *west*, abstracting for you some common tasks.

For ``west`` commands other than ``ardep`` refer to `Zephyrs documentation on west <https://docs.zephyrproject.org/latest/develop/west/index.html>`_.

See ``west ardep --help`` for a list of all existing commands.

See ``west ardep {subcommand} --help`` for more information on the individual command.


.. note::

    If you have an **Ardep board v2** or later, you won't need the commands:

    - ``west ardep create-udev-rule``
    - ``dfu``
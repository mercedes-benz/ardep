.. _iso14229-sample:
   
ISO14229 Sample
###############

This sample can be used to test the iso14229 library wrapper by changing the diagnostic session on the device.

Setup your can interface as described in the :ref:`uds-sample` and then run the client with:

    .. code-block:: bash

        python3 client.py --can <your_can_interface>


For more information on how to handle events, refer to the library tests.
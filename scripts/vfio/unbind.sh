#!/bin/bash

source vfio.sh

vfio_unbind 0000:01:00.0
vfio_unbind 0000:01:00.1
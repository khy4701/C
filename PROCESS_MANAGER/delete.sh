#!/bin/sh

ipcs

ipcrm -Q 9999
ipcrm -M 5678 

ipcs

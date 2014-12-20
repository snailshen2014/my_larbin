#!/bin/sh
ps -ef|grep larbin|awk '{print $2}'|xargs kill

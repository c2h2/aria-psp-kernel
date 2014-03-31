
check_error()
{
  if test "$1" != "0" ; then
    tput setaf 1
    echo "+  Error : ${1} "
    tput sgr0
    exit $1
  fi
}

check_warning()
{
    
  if test "$1" != "0" ; then
    tput setaf 3
    echo "+  Warning : ${1} "
    tput sgr0
  fi
}

print_error()
{
  if test "$1" = "" ; then return ; fi
  tput setaf 1
  echo $1
  tput sgr0
}

echo_error()
{
  if test "$1" = "" ; then return ; fi
  tput setaf 1
  echo $1
  tput sgr0
}

print_warning()
{
  if test "$1" = "" ; then return ; fi
  tput setaf 3
  echo $1
  tput sgr0
}

echo_warning()
{
  if test "$1" = "" ; then return ; fi
  tput setaf 3
  echo $1
  tput sgr0
}

print_success()
{
  if test "$1" = "" ; then return ; fi
   
  tput setaf 2
  echo $1
  tput sgr0
}

echo_success()
{
   
  if test "$1" = "" ; then return ; fi
  tput setaf 2
  echo $1
  tput sgr0
}

check_warning()
{
    
  if test "$1" != "0" ; then
    tput setaf 3
    echo "+  Warning : ${1} "
    tput sgr0
  fi
}

check_warning_and_success()
{
  if test "$1" != "0" ; then
    print_warning "---- $3 code: $1"
  else
    if test "$2" = "" ; then return ; fi
    print_success "---- $2"
  fi
    
}

check_error_and_success()
{
  if test "$1" != "0" ; then
    print_error "---- $3 code: $1"
    exit $1
  else
    if test "$2" = "" ; then return ; fi
    print_success "---- $2"
  fi
}

echo_stage()
{
  tput setaf 4
  echo $1
  tput sgr0
}




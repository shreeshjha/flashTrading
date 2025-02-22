module advanced_order_book
  use iso_c_binding
  implicit none

  integer, parameter :: max_instruments = 20
  integer, parameter :: max_orders      = 200
  integer, parameter :: max_trades      = 2000

  ! Order types: 0 = Limit, 1 = Market, 2 = Stop
  integer, parameter :: ORDER_LIMIT = 0, ORDER_MARKET = 1, ORDER_STOP = 2

  type, bind(C) :: trade
      integer(c_int) :: trade_id
      character(kind=c_char) :: symbol(8)
      real(c_double) :: price
      integer(c_int) :: quantity
      character(kind=c_char) :: side
  end type trade

  type, bind(C) :: order
      integer(c_int) :: id
      character(kind=c_char) :: symbol(8)
      real(c_double) :: price
      integer(c_int) :: quantity
      character(kind=c_char) :: side
      integer(c_int) :: timestamp
      integer(c_int) :: order_type  ! New field for order type
  end type order

  type, bind(C) :: instrument_book
      character(kind=c_char) :: symbol(8)
      type(order), dimension(max_orders) :: orders
      integer(c_int) :: order_count
      type(trade), dimension(max_trades) :: trades
      integer(c_int) :: trade_count
  end type instrument_book

  type(instrument_book), dimension(max_instruments), save, bind(C) :: books

  integer(c_int), save, bind(C) :: instrument_count = 0
  integer(c_int), save, bind(C) :: global_trade_id = 0

contains

  function cchar_to_string(arr) result(str)
      character(kind=c_char), intent(in) :: arr(*)
      character(len=8) :: str
      integer :: i
      str = ""
      do i = 1, 8
         if (arr(i) == c_null_char) exit
         str(i:i) = arr(i)
      end do
  end function cchar_to_string

  function find_instrument_index(symbol) result(idx)
    character(kind=c_char), intent(in) :: symbol(*)
    integer(c_int) :: idx
    integer :: i
    character(len=8) :: sym_fortran, temp_str

    sym_fortran = ''
    do i = 1, 8
        if (symbol(i) == c_null_char) exit
        sym_fortran(i:i) = symbol(i)
    end do
    sym_fortran = trim(adjustl(sym_fortran))
    print *, "DEBUG: looking for symbol [", trim(sym_fortran), "]"
    if (len_trim(sym_fortran) == 0) then
        print *, "Error: empty symbol"
        idx = -1
        return
    end if
    do i = 1, instrument_count
        temp_str = cchar_to_string(books(i)%symbol)
        temp_str = trim(adjustl(temp_str))
        print *, "DEBUG: comparing with [", trim(temp_str), "]"
        if (trim(temp_str) == trim(sym_fortran)) then
            idx = i
            print *, "DEBUG: found match at index", i
            return
        end if
    end do
    if (instrument_count < max_instruments) then
        instrument_count = instrument_count + 1
        do i = 1, len_trim(sym_fortran)
            books(instrument_count)%symbol(i:i) = sym_fortran(i:i)
        end do
        do i = len_trim(sym_fortran) + 1, 8
            books(instrument_count)%symbol(i:i) = ' '
        end do
        books(instrument_count)%order_count = 0
        books(instrument_count)%trade_count = 0
        idx = instrument_count
        print *, "Created new instrument slot for [", trim(sym_fortran), "] at index", idx
    else
        print *, "Instrument limit reached for symbol [", trim(sym_fortran), "]"
        idx = -1
    end if
  end function find_instrument_index

  subroutine record_trade(idx, side, price, quantity)
      integer(c_int), value :: idx, quantity
      character(kind=c_char), value :: side
      real(c_double), value :: price
      integer(c_int) :: tc
      global_trade_id = global_trade_id + 1
      tc = books(idx)%trade_count
      if (tc < max_trades) then
          tc = tc + 1
          books(idx)%trades(tc)%trade_id = global_trade_id
          books(idx)%trades(tc)%symbol = books(idx)%symbol
          books(idx)%trades(tc)%price = price
          books(idx)%trades(tc)%quantity = quantity
          books(idx)%trades(tc)%side = side
          books(idx)%trade_count = tc
      end if
  end subroutine record_trade

  subroutine match_order(idx, o)
      integer(c_int), value :: idx
      type(order) :: o
      integer :: i
      integer(c_int) :: match_idx
      real(c_double) :: match_price
      integer(c_int) :: fill_qty
      character(len=8) :: o_str, temp_str

      o_str = cchar_to_string(o%symbol)
      do
          if (o%quantity <= 0) exit
          match_idx = -1
          if (o%order_type == ORDER_MARKET) then
              if (o%side == 'B') then
                  match_price = 1.0d15
                  do i = 1, books(idx)%order_count
                      temp_str = cchar_to_string(books(idx)%orders(i)%symbol)
                      if (books(idx)%orders(i)%side == 'S' .and. trim(temp_str) == trim(o_str)) then
                          if (books(idx)%orders(i)%price < match_price) then
                              match_price = books(idx)%orders(i)%price
                              match_idx = i
                          end if
                      end if
                  end do
              else
                  match_price = -1.0d0
                  do i = 1, books(idx)%order_count
                      temp_str = cchar_to_string(books(idx)%orders(i)%symbol)
                      if (books(idx)%orders(i)%side == 'B' .and. trim(temp_str) == trim(o_str)) then
                          if (books(idx)%orders(i)%price > match_price) then
                              match_price = books(idx)%orders(i)%price
                              match_idx = i
                          end if
                      end if
                  end do
              end if
          else
              if (o%side == 'B') then
                  match_price = 1.0d15
                  do i = 1, books(idx)%order_count
                      temp_str = cchar_to_string(books(idx)%orders(i)%symbol)
                      if (books(idx)%orders(i)%side == 'S' .and. trim(temp_str) == trim(o_str)) then
                          if (books(idx)%orders(i)%price <= match_price) then
                              match_price = books(idx)%orders(i)%price
                              match_idx = i
                          end if
                      end if
                  end do
                  if (match_price > o%price) match_idx = -1
              else
                  match_price = -1.0d0
                  do i = 1, books(idx)%order_count
                      temp_str = cchar_to_string(books(idx)%orders(i)%symbol)
                      if (books(idx)%orders(i)%side == 'B' .and. trim(temp_str) == trim(o_str)) then
                          if (books(idx)%orders(i)%price >= match_price) then
                              match_price = books(idx)%orders(i)%price
                              match_idx = i
                          end if
                      end if
                  end do
                  if (match_price < o%price) match_idx = -1
              end if
          end if

          if (match_idx == -1) exit

          fill_qty = min(o%quantity, books(idx)%orders(match_idx)%quantity)
          call record_trade(idx, o%side, match_price, fill_qty)
          o%quantity = o%quantity - fill_qty
          books(idx)%orders(match_idx)%quantity = books(idx)%orders(match_idx)%quantity - fill_qty
          if (books(idx)%orders(match_idx)%quantity <= 0) then
              books(idx)%orders(match_idx) = books(idx)%orders(books(idx)%order_count)
              books(idx)%order_count = books(idx)%order_count - 1
          end if
      end do

      if (o%quantity > 0) then
          if (books(idx)%order_count < max_orders) then
              books(idx)%order_count = books(idx)%order_count + 1
              books(idx)%orders(books(idx)%order_count) = o
          end if
      end if
  end subroutine match_order

  subroutine add_order(id, symbol, price, quantity, side, order_type) bind(C, name="add_order")
      integer(c_int), value :: id, quantity, order_type
      character(kind=c_char), intent(in) :: symbol(*)
      real(c_double), value :: price
      character(kind=c_char), value :: side
      integer(c_int) :: idx
      type(order) :: o
      character(len=8) :: sym_fortran
      integer :: i

      sym_fortran = ""
      do i = 1, 8
         sym_fortran(i:i) = symbol(i)
      end do

      idx = find_instrument_index(symbol)
      if (idx == -1) then
         print *, "Instrument limit reached or error for symbol [", trim(sym_fortran), "]"
         return
      end if

      o%id = id
      o%symbol = sym_fortran
      o%price = price
      o%quantity = quantity
      o%side = side
      o%timestamp = 0
      o%order_type = order_type
      call match_order(idx, o)
  end subroutine add_order

  subroutine cancel_order(symbol, id, status) bind(C, name="cancel_order")
      character(kind=c_char), intent(in) :: symbol(*)
      integer(c_int), value :: id
      integer(c_int), intent(out) :: status
      integer :: idx, i
      character(len=8) :: sym_fortran

      sym_fortran = ""
      do i = 1, 8
         sym_fortran(i:i) = symbol(i)
      end do

      status = 1
      idx = find_instrument_index(symbol)
      if (idx == -1) return

      do i = 1, books(idx)%order_count
          if (books(idx)%orders(i)%id == id) then
              books(idx)%orders(i) = books(idx)%orders(books(idx)%order_count)
              books(idx)%order_count = books(idx)%order_count - 1
              status = 0
              exit
          end if
      end do
  end subroutine cancel_order

  subroutine modify_order(symbol, id, new_price, new_quantity, status) bind(C, name="modify_order")
      character(kind=c_char), intent(in) :: symbol(*)
      integer(c_int), value :: id, new_quantity
      real(c_double), value :: new_price
      integer(c_int), intent(out) :: status
      integer :: idx, i
      character(len=8) :: sym_fortran
      character(kind=c_char) :: side
      integer(c_int) :: order_type

      sym_fortran = ""
      do i = 1, 8
         sym_fortran(i:i) = symbol(i)
      end do

      status = 1
      idx = find_instrument_index(symbol)
      if (idx == -1) return

      do i = 1, books(idx)%order_count
          if (books(idx)%orders(i)%id == id) then
              side = books(idx)%orders(i)%side
              order_type = books(idx)%orders(i)%order_type
              books(idx)%orders(i) = books(idx)%orders(books(idx)%order_count)
              books(idx)%order_count = books(idx)%order_count - 1
              status = 0
              exit
          end if
      end do

      if (status == 0) then
         call add_order(id, symbol, new_price, new_quantity, side, order_type)
      end if
  end subroutine modify_order

  subroutine get_order_count(symbol, count) bind(C, name="get_order_count")
      character(kind=c_char), intent(in) :: symbol(*)
      integer(c_int), intent(out) :: count
      integer(c_int) :: idx

      idx = find_instrument_index(symbol)
      if (idx == -1) then
         count = -1
      else
         count = books(idx)%order_count
      end if
  end subroutine get_order_count

  subroutine get_order_book_snapshot(symbol, out_prices, out_qtys, out_sides, out_count) bind(C, name="get_order_book_snapshot")
      character(kind=c_char), intent(in) :: symbol(*)
      real(c_double), intent(out) :: out_prices(max_orders)
      integer(c_int), intent(out) :: out_qtys(max_orders)
      character(kind=c_char), intent(out) :: out_sides(max_orders)
      integer(c_int), intent(out) :: out_count
      integer :: idx, i

      idx = find_instrument_index(symbol)
      if (idx == -1) then
         out_count = 0
         return
      end if

      out_count = books(idx)%order_count
      do i = 1, out_count
         out_prices(i) = books(idx)%orders(i)%price
         out_qtys(i)   = books(idx)%orders(i)%quantity
         out_sides(i)  = books(idx)%orders(i)%side
      end do
  end subroutine get_order_book_snapshot

  subroutine get_trades(symbol, out_prices, out_qtys, out_sides, out_tids, out_count) bind(C, name="get_trades")
      character(kind=c_char), intent(in) :: symbol(*)
      real(c_double), intent(out) :: out_prices(max_trades)
      integer(c_int), intent(out) :: out_qtys(max_trades), out_tids(max_trades)
      character(kind=c_char), intent(out) :: out_sides(max_trades)
      integer(c_int), intent(out) :: out_count
      integer :: idx, i

      idx = find_instrument_index(symbol)
      if (idx == -1) then
         out_count = 0
         return
      end if

      out_count = books(idx)%trade_count
      if (out_count > max_trades) out_count = max_trades

      do i = 1, out_count
         out_prices(i) = books(idx)%trades(i)%price
         out_qtys(i)   = books(idx)%trades(i)%quantity
         out_sides(i)  = books(idx)%trades(i)%side
         out_tids(i)   = books(idx)%trades(i)%trade_id
      end do
  end subroutine get_trades

  subroutine get_risk_metrics(symbol, total_qty) bind(C, name="get_risk_metrics")
      character(kind=c_char), intent(in) :: symbol(*)
      integer(c_int), intent(out) :: total_qty
      integer :: idx, i
      total_qty = 0
      idx = find_instrument_index(symbol)
      if (idx == -1) return
      do i = 1, books(idx)%order_count
          total_qty = total_qty + books(idx)%orders(i)%quantity
      end do
  end subroutine get_risk_metrics

end module advanced_order_book


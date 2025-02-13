module order_book
    use iso_c_binding
    implicit none
    
    integer, parameter :: max_orders = 100  ! Changed from max to max_orders to match usage
    
    ! Define the order type
    type, bind(C) :: order
        integer(c_int) :: id 
        real(c_double) :: price 
        integer(c_int) :: quantity
        character(kind=c_char, len=1) :: side ! 'B' for buy, 'S' for sell
    end type order 
    
    ! Module-level variables
    type(order), dimension(max_orders) :: orders
    integer(c_int) :: order_count = 0

contains
    ! Adds an order to the order book. 
    subroutine add_order(id, price, quantity, side) bind(C, name="add_order")
        integer(c_int), value :: id 
        real(c_double), value :: price 
        integer(c_int), value :: quantity
        character(kind=c_char), value :: side
        
        if(order_count < max_orders) then
            order_count = order_count + 1 
            orders(order_count)%id = id 
            orders(order_count)%price = price 
            orders(order_count)%quantity = quantity
            orders(order_count)%side = side 
        end if 
    end subroutine add_order

    ! Returns the current number of orders.
    subroutine get_order_count(count) bind(C, name="get_order_count")
        integer(c_int), intent(out) :: count 
        count = order_count 
    end subroutine get_order_count

end module order_book

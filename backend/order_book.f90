module order_book
    use iso_c_binding
    implicit none
    
    integer, parameter :: max_orders = 100
    
    ! Define the order type
    type, bind(C) :: order
        integer(c_int) :: id
        real(c_double) :: price
        integer(c_int) :: quantity
        character(kind=c_char) :: side
    end type order
    
    ! Make these variables persist and expose them to C
    type(order), dimension(max_orders), save, bind(C) :: orders
    integer(c_int), save, bind(C) :: order_count = 0

contains

    subroutine add_order(id, price, quantity, side) bind(C, name="add_order")
        integer(c_int), value :: id
        real(c_double), value :: price
        integer(c_int), value :: quantity
        character(kind=c_char), value :: side
        
        if (order_count < max_orders) then
            order_count = order_count + 1
            orders(order_count)%id = id
            orders(order_count)%price = price
            orders(order_count)%quantity = quantity
            orders(order_count)%side = side
            print *, "Added order:", id, "Count:", order_count  ! Debug print
        end if
    end subroutine add_order

    subroutine get_order_count(count) bind(C, name="get_order_count")
        integer(c_int), intent(out) :: count
        count = order_count
        print *, "Getting count:", order_count  ! Debug print
    end subroutine get_order_count

    subroutine cancel_order(id, status) bind(C, name="cancel_order")
        integer(c_int), value :: id 
        integer(c_int), intent(out) :: status 
        integer :: i, found 
        found = 0
        do i = 1, order_count 
            if (orders(i)%id == id) then 
                found = 1 
                exit 
            end if 
        end do 
        if (found == 1) then 
            orders(i) = orders(order_count)
            order_count = order_count - 1 
            status = 0
            print *, "Order not found:", id, "New count:", order_count
        else 
            status = 1 
            print *, "Order not found:", id 
        end if 
    end subroutine cancel_order

    subroutine modify_order(id, new_price, new_quantity, status) bind(C, name="modify_order")
        integer(c_int), value :: id 
        real(c_double), value :: new_price
        integer(c_int), value :: new_quantity
        integer(c_int), intent(out) :: status 
        integer :: i, found 
        found = 0
        do i = 1, order_count
            if (orders(i)%id == id) then
                found = 1
                orders(i)%price = new_price
                orders(i)%quantity = new_quantity
                exit 
            end if 
        end do 
        if (found == 1) then 
            status = 0 
            print *, "Modified order:", id, "New price:", new_price, "New quantity:", new_quantity
        else 
            status = 1 
            print *, "Order not found for modification:", id 
        end if 
    end subroutine modify_order
end module order_book

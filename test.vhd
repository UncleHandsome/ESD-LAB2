-- ########################################################################### 
-- ##### Slave Bus Interface (DATA) 
-- ########################################################################### 

PORT_WR : process (GSR1, IO_nWE) 
begin 
    if(GSR1 = '1')then 
        CFG_IO_BASE <= X"0"; 
        CFG_IO_MODE <= MODE_PS2; 
        REG_CTRL1 <= X"07"; 
        REG_CTRL2 <= X"10"; 
        REG_CTRL3 <= X"FF"; 
    elsif(rising_edge(IO_nWE)) then 
        if(IO_nECS3 = '0') then 
            case IO_ADDR(11 downto 0) is 
                when X"FF0" => CFG_IO_BASE <= IO_DATA(3 downto 0); 
                when X"FF1" => CFG_IO_MODE <= IO_DATA(3 downto 0); 
                when X"000" => REG_CTRL1 <= IO_DATA(7 downto 0); 
                when X"001" => REG_CTRL2 <= IO_DATA(7 downto 0); 
                when X"002" => REG_CTRL3 <= IO_DATA(7 downto 0); when others => null ; 
            end case; 
        end if; 
    end if; 
end process; 


MUX_DATA <= X"FF" & MEMD when(IO_nRCS3='0' and IO_nOE = '0') else 
            X"000"& CFG_IO_BASE when(IO_ADDR(11 downto 0) = X"FF0") else X"000"& CFG_IO_MODE when(IO_ADDR(11 downto 0) = X"FF1") else 
            X"00" & REG_CTRL1 when(IO_ADDR(11 downto 0) = X"000") else 
            X"00" & REG_CTRL2 when(IO_ADDR(11 downto 0) = X"001") else 
            X"00" & REG_CTRL3 when(IO_ADDR(11 downto 0) = X"002") else 
            COUNTER(15 downto 0) when(IO_ADDR(11 downto 0) = X"003") else 
            COUNTER(31 downto 16) when(IO_ADDR(11 downto 0) = X"004") else X"00" & PS2_DATA_O when(IO_ADDR(11 downto 8) = X"1") else 
            X"00" & A2D_DATA_O when(IO_ADDR(11 downto 8) = X"2") else 
            tx_dat when(IO_ADDR(11 downto 8) = X"3") else 
            rx_dat when(IO_ADDR(11 downto 8) = X"4") else X"00" & uart_dat when(IO_ADDR(11 downto 8) = X"5") else 
            X"BEEF"; 


PORT_RD : process(IO_nECS3, IO_nRCS3, IO_nOE, MUX_DATA) 
begin 
    if ((IO_nECS3='0' or IO_nRCS3='0') and IO_nOE = '0') then 
        IO_DATA <= MUX_DATA; 
    else 
        IO_DATA <= "ZZZZZZZZZZZZZZZZ"; 
    end if; 
end process;

function CaveNet.ActServer()
	print("CaveNet.eventCode: " .. CaveNet.eventCode())
	if CaveNet.eventCode() == 20 then
		print("Sending a response back hopefully?")
		CaveNet.ENet.sendPacketID(20)
	end
	
	if CaveNet.eventCode() == 22 then
		print("starting a tsc command on all clients LOL!")
		CaveNet.ENet.sendPacketID(22)
	end
	
	if CaveNet.specialEventCode() == 1 then
		print("Hell yeah! Custom data! gimme that")
		print(CaveNet.GetSpecialPacketData())
		CaveNet.ENet.sendSpecialPacket(1, "Hey its me the server give me $1000 so i can stay up <3")
	end
end
/** 
 * This file is part of project URSA. More information on the project
 * can be found at 
 *
 * URSA's repository at GitHub: http://https://github.com/andersondomingues/ursa
 *
 * Copyright (C) 2018 Anderson Domingues, <ti.andersondomingues@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. **/
#include <NocRouterModel.h>
#include <cstdlib>

/**
 * @brief Ctor.
 * @param name Name of the instance (proccess impl)
 * @param x_pos X coordinate (first part of router addr)
 * @param y_pos Y coordinate (second part of router addr) */
NocRouterModel::NocRouterModel(std::string name, uint32_t x_pos, uint32_t y_pos) : Process(name){
    _x = x_pos;
    _y = y_pos;
    
    for(int i = 0; i < 5; i++){
        std::string bname = "(" + std::to_string(_x) + "," + std::to_string(_y) + ").OUT" + std::to_string(i);
        _ob[i] = new Buffer(bname);
        _ib[i] = nullptr;
    }
    
    this->Reset();
}

void NocRouterModel::Reset(){
    _round_robin = LOCAL; //starts checking on local port
    _state = RouterState::ROUNDROBIN;
    _packets_to_send = 0;
}

/**
 * @brief Implementation of the Run method from
 * the Proccess abstract class.
 * @return The next time to schedule the event.*/
unsigned long long NocRouterModel::Run(){
    
    //LOGIC: if in wormhole mode, keep sending 
    //packets until (or skiping) until the wormhole
    //goes off. If not in wormwhole mode, route.
    switch(_state){
        case RouterState::WORMHOLE:
        {
            //if packets to be sent, 
            Buffer* ob = _ob[_target_port];
            Buffer* ib = _ib[_source_port];
            
            std::cout << "sent from (" << _x << "," << _y << ") at (" << _target_port << ")"<< std::endl;
            
            ob->push(ib->top());
            ib->pop();
            
            _packets_to_send--;
            
            if(_packets_to_send == 0)
                _state = RouterState::ROUNDROBIN;

            break;
        }
        case RouterState::ROUNDROBIN:
        {
            Buffer* tb = _ib[_round_robin];

            //prevent from serving unconnected ports (e.g. border)
            if(tb != nullptr){
                if(tb->size() > 0){  //if has packet to send
                    _state = RouterState::WORMHOLE;
                    _source_port = _round_robin;
                    _target_port = this->GetRoute(tb->top()); 
                    _packets_to_send = tb->size();
                }
            }
            //std::cout << "rr";
            _round_robin++;
            _round_robin = _round_robin % 5; //get next port
            
            break;
        }   
            
    }
    
    return 1;
}

/**
 * @brief Calculate the port to route a given flit
 * @param flit to be routed
 * @return the port to where te packet must go*/
uint32_t NocRouterModel::GetRoute(uint32_t flit){
    
    uint32_t tx = (flit & 0xFF000000) >> 24;
    uint32_t ty = (flit & 0x00FF0000) >> 16;

    //if X=0, then route "vertically" (Y)
    if(_x == tx){
    
        return (_y == ty)
            ? LOCAL
            : (_y > ty)
                ? SOUTH
                : NORTH;      
    //else route X
    }else{

        return (_x > tx)
            ? WEST
            : EAST;
    }
}

/**
 * @brief Free allocated memory if any
 */
NocRouterModel::~NocRouterModel(){
    //nothing to do here
}


/**
 * @brief Get a pointer to one of the output buffers.
 * @param r The port from which get the pointer.
 * @return A pointer to the buffer.*/
Buffer* NocRouterModel::GetOutputBuffer(uint32_t r){
    return _ob[r];
}

Buffer* NocRouterModel::GetInputBuffer(uint32_t r){
    return _ib[r];
}

void NocRouterModel::SetInputBuffer(Buffer* b, uint32_t port){
    _ib[port] = b;
}
/* ----------------------------------------------------------------------- *
 *
 *   Copyright 2011 Erwan Velu - All Rights Reserved
 *
 *   Permission is hereby granted, free of charge, to any person
 *   obtaining a copy of this software and associated documentation
 *   files (the "Software"), to deal in the Software without
 *   restriction, including without limitation the rights to use,
 *   copy, modify, merge, publish, distribute, sublicense, and/or
 *   sell copies of the Software, and to permit persons to whom
 *   the Software is furnished to do so, subject to the following
 *   conditions:
 *
 *   The above copyright notice and this permission notice shall
 *   be included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 *
 * -----------------------------------------------------------------------
 */

#include "hdt-common.h"
#include "hdt-dump.h"
#include <sys/gpxe.h>

void dump_pxe(struct s_hardware *hardware, ZZJSON_CONFIG *config, ZZJSON **item) {

	if (hardware->is_pxe_valid == false) return;
        *item = zzjson_create_object(config, NULL); /* empty object */
	char buffer[32] = {0};
	snprintf(buffer,sizeof(buffer),"0x%x",hardware->pxe.vendor_id);
	add_s("pxe.vendor_id",buffer);
	snprintf(buffer,sizeof(buffer),"0x%x",hardware->pxe.product_id);
	add_s("pxe.product_id",buffer);
	snprintf(buffer,sizeof(buffer),"0x%x",hardware->pxe.subvendor_id);
	add_s("pxe.subvendor_id",buffer);
	snprintf(buffer,sizeof(buffer),"0x%x",hardware->pxe.subproduct_id);
	add_s("pxe.subproduct_id",buffer);

	if (hardware->pci_ids_return_code == -ENOPCIIDS || (hardware->pxe.pci_device == NULL)) { 
		add_s("Manufacturer_name","no_pci_ids_file or no device found");
		add_s("Product_name","no_pci_ids_file or no device found");
	} else {
		add_s("Manufacturer_name", hardware->pxe.pci_device->dev_info->vendor_name);
		add_s("Product_name", hardware->pxe.pci_device->dev_info->product_name);
	}

	add_hi(pxe.rev);
	add_hi(pxe.pci_bus);
	add_hi(pxe.pci_dev);
	add_hi(pxe.pci_func);
	add_hi(pxe.base_class);
	add_hi(pxe.sub_class);
	add_hi(pxe.prog_intf);
	add_hi(pxe.nictype);
	add_hs(pxe.mac_addr);
	char ip[16] = {0};
	snprintf(ip,sizeof(ip), "%d.%d.%d.%d",
			hardware->pxe.ip_addr[0], 
			hardware->pxe.ip_addr[1],
			hardware->pxe.ip_addr[2],
			hardware->pxe.ip_addr[3]);
	add_s("pxe.ipaddr",ip);
	add_b("gpxe_detected",is_gpxe());
	flush("pxe",config,item);
}

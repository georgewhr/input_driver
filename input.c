/*
 *  SVU Intro to Embedded Systems Lab>
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


#include <linux/module.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/mod_devicetable.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>

MODULE_LICENSE("Dual BSD/GPL");

#define HW_RAW(dev) (test_bit(EV_MSC,dev->evbit) && test_bit(MSC_RAW,dev->mscbit) && ((dev)->id.bustype == BUS_I8042) && ((dev)->id.vendor == 0x0001) && ((dev)->id.product == 0x0001))
#define DEFAULT_SPEED 10
#define MAX_SPEED 50
#define MIN_SPEED 1
struct vms
{
        struct input_handler *handler;
        struct input_handle *handle;
        struct input_dev *dev;
        struct work_struct work;
        spinlock_t lock;
        unsigned int event_code;
        int value;
}*vms;



void vms_func(struct work_struct *work)
{
        spin_lock(&vms->lock);

        switch(vms->event_code)
        {

                case KEY_S:
                        printk(KERN_ALERT "S is typed\n");
                        break;

                case KEY_V:
                        printk(KERN_ALERT "V is typed\n");
                        break;

                case KEY_U:
                        printk(KERN_ALERT "U is typed\n");
                        break;
                default:
                        printk(KERN_ALERT "The button you typed is %d, value is %d\n", vms->event_code, vms->value);
                        break;

       }
        spin_unlock(&vms->lock);

}

static void vkbd_event(struct input_handle *handle, unsigned int event_type,
                        unsigned int event_code, int value)
{
        if(event_type == EV_KEY)
        {
                vms->event_code = event_code;
                vms->value = value;
                schedule_work(&vms->work);
        }

}

static int vkbd_connect(struct input_handler *handler, struct input_dev *dev,
                        const struct input_device_id *id)
{
        struct input_handle *handle;
        int error;
        int i;
       
        for(i = KEY_RESERVED; i < BTN_MISC; i++)
                if(test_bit(i,dev->keybit))
                        break;
       
        if(i == BTN_MISC && !test_bit(EV_SND, dev->evbit))
                return -ENODEV;
       
        handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
        vms->handle = handle;
        if(!handle)
                return -ENOMEM;
       
        handle->dev = dev;
        handle->handler = handler;
        handle->name = "vkbd";

        error = input_register_handle(handle);
        if(error)
                goto err_free_handle;
       
        error = input_open_device(handle);
        if(error)
                goto err_unregister_handle;
       
        printk(KERN_ALERT "vkbd_connect: %s\n", dev->name);
        return 0;

err_unregister_handle:
        input_unregister_handle(handle);
err_free_handle:
        kfree(handle);
        return error;
}

static void vkbd_disconnect(struct input_handle *handle)
{
        input_close_device(handle);
        input_unregister_handle(handle);
        kfree(handle);
}
	
static void vkbd_start(struct input_handle *handle)
{
        input_inject_event(handle,EV_LED,LED_CAPSL,0x04);
        input_inject_event(handle,EV_SYN,SYN_REPORT,0);
}

static const struct input_device_id vkbd_ids[] =
{
        {
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT,
        .evbit = { BIT_MASK(EV_KEY) },
        },
        {
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT,
        .evbit = { BIT_MASK(EV_SND) },
        },

        {
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT,
        .evbit = { BIT_MASK(EV_REL) },
        },

        {
        },
};

static struct input_handler vkbd_handler =
{
        .event           = vkbd_event,
        .connect         = vkbd_connect,
        .disconnect      = vkbd_disconnect,
        .start           = vkbd_start,
        .name            = "lab_irq",
        .id_table        = vkbd_ids,
};

int __init vms_init(void)
{
        int error;

        vms = kzalloc(sizeof(struct vms),GFP_KERNEL);
        vms->handler = &vkbd_handler;
        
        /*
         *  This function will initialize thw workquese, the work quese is essentially a kernel thread
         */

        INIT_WORK(&vms->work,vms_func);
       
        spin_lock_init(&vms->lock);

        error = input_register_handler(&vkbd_handler);
        if(error)
                return error;

        //init vms_dev
        vms->dev = input_allocate_device();
        if(!vms->dev)
        {
                printk(KERN_ALERT "vms: Bad input_allocate_device()\n");
                return -1;
        }

        vms->dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
        // vms->dev->keybit[BIT_WORD(BTN_LEFT)] = BIT_MASK(BTN_LEFT);

        vms->dev->keybit[BIT_WORD(BTN_MOUSE)] = BIT_MASK(BTN_LEFT) |
                BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_MIDDLE);
        vms->dev->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y);
       
        vms->dev->name = "vms";
        vms->dev->phys = "vms";
        vms->dev->id.bustype = BUS_I8042;
        vms->dev->id.vendor = 0x0002;
        vms->dev->id.product = 5;
        vms->dev->id.version = 0;
 
        input_register_device(vms->dev);

        printk(KERN_ALERT "vms init\n");
        return 0;
}

void __exit vms_exit(void)
{
        if(vms->dev)
                input_unregister_device(vms->dev);
        input_unregister_handler(vms->handler);
        kfree(vms);
}

module_init(vms_init);
module_exit(vms_exit);

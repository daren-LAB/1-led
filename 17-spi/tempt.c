
static int w25q80_read_regs(struct wq_dev *dev,u8 reg,void *buf,int len)
{
    int ret = 0;
    unsigned char txdata[len];
    struct spi_message m;
    struct spi_transfer *t;
    struct spi_device *spi = (struct spi_device *)dev->private_data;

    gpio_set_value(dev->cs_gpio, 0);
    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
    txdata[0] = reg |0x80;
    t->tx_buf = txdata;
    t->len = 1;
    spi_message_init(&m);
    spi_message_add_tail(t,&m);
    ret = spi_sync(spi,&m);

    txdata[0] = 0xff;
    t->rx_buf = buf;
    t->len = len;
    spi_message_init(&m);
    spi_message_add_tail(t,&m);
    ret = spi_sync(spi,&m);

    kfree(t);
    gpio_set_value(dev->cs_gpio,1);
    return ret;
}
static int w25q80_write_regs(struct wq_dev *dev,u8 reg,void *buf,int len)
{
    int ret = 0;
    unsigned char txdata[len];
    struct spi_message m;
    struct spi_transfer *t;
    struct spi_device *spi = (struct spi_device *)dev->private_data;

    gpio_set_value(dev->cs_gpio, 0);
    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
    txdata[0] = reg & ~ 0x80;
    t->tx_buf = txdata;
    t->len = 1;
    spi_message_init(&m);
    spi_message_add_tail(t,&m);
    ret = spi_sync(spi,&m);

    t->tx_buf = buf;
    t->len = len;
    spi_message_init(&m);
    spi_message_add_tail(t,&m);
    ret = spi_sync(spi,&m);

    kfree(t);
    gpio_set_value(dev->cs_gpio,1);
    return ret;
}
static unsigned char w25q80_read_onereg(struct wq_dev *dev,u8 reg)
{
 u8 data = 0;
 w25q80_read_regs(dev, reg, &data, 1);
 return data;
}
static void w25q80_write_onereg(struct wq_dev *dev, u8 reg,u8 value)
 {
 u8 buf = value;
 w25q80_write_regs(dev, reg, &buf, 1);
 }
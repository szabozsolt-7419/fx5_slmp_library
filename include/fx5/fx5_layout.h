#ifndef FX5_LAYOUT_H
#define FX5_LAYOUT_H

/** @brief Request TCP header size for 3E binary frames. */
#define FX5_REQ_TCP_HEADER_SIZE_3E           (11u)

/** @brief Request TCP header size for 4E binary frames. */
#define FX5_REQ_TCP_HEADER_SIZE_4E           (15u)

/** @brief Response TCP header size for 3E binary frames. */
#define FX5_RESP_TCP_HEADER_SIZE_3E          (9u)

/** @brief Response TCP header size for 4E binary frames. */
#define FX5_RESP_TCP_HEADER_SIZE_4E          (13u)

/** @brief Common request PDU header size. */
#define FX5_REQ_PDU_HEADER_SIZE              (10u)


/** @brief Common response PDU header size. */
#define FX5_RESP_PDU_HEADER_SIZE             (2u)

/** @brief Additional guard bytes for the internal RX/TX buffer. */
#define FX5_IOBUF_GUARD_BYTES                (16u)


#endif /* FX5_LAYOUT_H */